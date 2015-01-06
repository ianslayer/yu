#include "../math/vector.h"
#include "../container/dequeue.h"
#include "../core/allocator.h"
#include "../core/system.h"
#include "../core/log.h"
#include "../core/thread.h"
#include "../core/timer.h"
#include "../core/string.h"
#include "resource_ptr_dx.h"
#include "shader_dx11.h"
#include "renderer_impl.h"

#include <new>
#include <d3d11.h>

typedef HRESULT WINAPI  LPCREATEDXGIFACTORY(REFIID, void ** );
typedef HRESULT WINAPI  LPD3D11CREATEDEVICE(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );

#define SAFE_RELEASE(p) \
		{\
		ULONG testRef = 0;\
		if(p)\
			testRef = p->Release();\
		p = 0;\
		}

namespace yu
{
HMODULE hModuleDX11;
HMODULE hModuleDXGI;
LPCREATEDXGIFACTORY*    dllCreateDXGIFactory1;
LPD3D11CREATEDEVICE*    dllD3D11CreateDevice;

Thread				renderThread;


struct MeshDataDx11
{
	DxResourcePtr<ID3D11Buffer>			vertexBuffer;
	DxResourcePtr<ID3D11Buffer>			indexBuffer;
	DxResourcePtr<ID3D11InputLayout>	inputLayout;
};

struct CameraConstant
{
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
};

struct CameraDataDx11
{
	DxResourcePtr<ID3D11Buffer> constantBuffer;
};

struct VertexShaderDx11
{
	DxResourcePtr<ID3D11VertexShader> shader;
};

struct PixelShaderDx11
{
	DxResourcePtr<ID3D11PixelShader> shader;
};

struct PipelineDx11
{
	DxResourcePtr<ID3D11VertexShader> vs;
	DxResourcePtr<ID3D11PixelShader> ps;
};

struct RendererDx11 : public Renderer
{
	MeshDataDx11		dx11MeshList[MAX_MESH];
	CameraDataDx11		dx11CameraList[MAX_CAMERA];
	VertexShaderDx11	dx11VertexShaderList[MAX_SHADER];
	PixelShaderDx11		dx11PixelShaderList[MAX_SHADER];
	PipelineDx11		dx11PipelineList[MAX_PIPELINE];
};

RendererDx11* gRenderer;

Renderer* CreateRenderer()
{
	RendererDx11* renderer = New<RendererDx11>(gSysArena);
	gRenderer = renderer;
	return renderer;
}

void FreeRenderer(Renderer* renderer)
{
	RendererDx11* dx11Renderer = (RendererDx11*)renderer;

	dx11Renderer->~RendererDx11();
}

struct RenderDeviceDx11
{
	IDXGIFactory1*			dxgiFactory1 = nullptr;
	
	IDXGIAdapter1*			dxgiAdapter = nullptr;
	IDXGIOutput*			dxgiOutput = nullptr;
	IDXGISwapChain*			dxgiSwapChain = nullptr;

	ID3D11Device*			d3d11Device = nullptr;
	ID3D11DeviceContext*	d3d11DeviceContext = nullptr;
	SpscFifo<RenderThreadCmd, 16> renderThreadCmdQueue;
	bool					goingFullScreen = false;

	FrameBufferDesc			frameBufferDesc;
};

static RenderDeviceDx11* gDx11Device;

DXGI_FORMAT DXGIFormat(TexFormat fmt)
{
	switch(fmt)
	{
		case TEX_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEX_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}

	return DXGI_FORMAT_UNKNOWN;
}

TexFormat TexFormatFromDxgi(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM: return TEX_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return TEX_FORMAT_R8G8B8A8_UNORM_SRGB;
	}

	return TEX_FORMAT_UNKNOWN;
}

struct InitDX11Param
{
	Window			win;
	FrameBufferDesc	desc;
	CondVar			initDx11CV;
	Mutex			initDx11CS;
};

static bool SetFullScreen(const FrameBufferDesc& desc)
{
	if (desc.fullScreen)
	{
		DXGI_MODE_DESC desiredMode;
		DXGI_MODE_DESC matchMode;
		desiredMode.Format = DXGIFormat(desc.format);
		desiredMode.Width = desc.width;
		desiredMode.Height = desc.height;
		desiredMode.RefreshRate.Numerator = (UINT)floor(desc.refreshRate + 0.5);
		desiredMode.RefreshRate.Denominator = 1;
		desiredMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desiredMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
		HRESULT hr = gDx11Device->dxgiOutput->FindClosestMatchingMode(&desiredMode, &matchMode, gDx11Device->d3d11Device);

		gDx11Device->goingFullScreen = true;

		if (SUCCEEDED(hr))
		{
			hr = gDx11Device->dxgiSwapChain->ResizeTarget(&matchMode);
			if (SUCCEEDED(hr))
			{
				HRESULT switchHr = gDx11Device->dxgiSwapChain->SetFullscreenState(TRUE, gDx11Device->dxgiOutput); //FIXME: should wait until switch process is done

				if (SUCCEEDED(switchHr))
				{
					Log("switch to full screen\n");
					return false;
				}
			}
		}
		else
		{
			Log("failed to enter fullscreen mode\n");
			return false;
		}

		gDx11Device->goingFullScreen = false;
	}

	return false;
}

static bool InitDX11(const Window& win, const FrameBufferDesc& desc)
{
	gDx11Device = new RenderDeviceDx11();

	gDx11Device->frameBufferDesc = desc;

	hModuleDX11 = LoadLibraryA("d3d11.dll");

	if (hModuleDX11 != NULL)
	{
		dllD3D11CreateDevice = (LPD3D11CREATEDEVICE*)GetProcAddress(hModuleDX11, "D3D11CreateDevice");
		if (dllD3D11CreateDevice == NULL)
		{
			Log("error: failed to load D3D11CreateDevice\n");
			return false;
		}
	}
	else
	{
		Log("error: can't find d3d11.dll\n");
		return false;
	}

	hModuleDXGI = LoadLibraryA("dxgi.dll");

	if (hModuleDXGI != NULL)
	{
		dllCreateDXGIFactory1 = (LPCREATEDXGIFACTORY*)GetProcAddress(hModuleDXGI, "CreateDXGIFactory1");
		if (dllCreateDXGIFactory1 == NULL)
		{
			Log("error: failed to load CreateDXGIFactory1");
			return false;
		}
	}
	else
	{
		Log("error: can't find dxgi.dll\n");
		return false;
	}

	HRESULT hr;
	hr = dllCreateDXGIFactory1(__uuidof(IDXGIFactory1), (LPVOID*)&gDx11Device->dxgiFactory1);

	if (FAILED(hr))
	{
		Log("error: failed to create DXGIFactory1\n");
		return false;
	}

	Display mainDisplay = SystemInfo::GetMainDisplay();

	IDXGIAdapter1* adapter = NULL;

	for (UINT i = 0; gDx11Device->dxgiFactory1->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);

		Log("adapter desc: %ls\n", adapterDesc.Description);


		for (UINT j = 0; adapter->EnumOutputs(j, &gDx11Device->dxgiOutput) != DXGI_ERROR_NOT_FOUND; j++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			gDx11Device->dxgiOutput->GetDesc(&outputDesc);

			UINT numDispMode = 0;
			hr = gDx11Device->dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numDispMode, NULL);

			DXGI_MODE_DESC* pModeList = new DXGI_MODE_DESC[numDispMode];

			hr = gDx11Device->dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numDispMode, pModeList);
			Log("output mode:\n");
			for (UINT m = 0; m < numDispMode; m++)
			{

				Log("width:%d, height:%d, refresh rate: %lf \n", pModeList[m].Width, pModeList[m].Height, (double)pModeList[m].RefreshRate.Numerator / (double)pModeList[m].RefreshRate.Denominator);
			}

			delete[] pModeList;

			Log("output device: %ls\n", outputDesc.DeviceName);

			if (outputDesc.Monitor == mainDisplay.hMonitor)
			{
				Log("main display\n");

				D3D_FEATURE_LEVEL FeatureLevel;
				D3D_FEATURE_LEVEL FeatureLevels[1] = { D3D_FEATURE_LEVEL_11_0 };

				hr = dllD3D11CreateDevice(adapter,
					D3D_DRIVER_TYPE_UNKNOWN,
					//driverType, 
					0,
					D3D11_CREATE_DEVICE_DEBUG, //desc.directXDebugInfo == true?D3D11_CREATE_DEVICE_DEBUG:0, 
					FeatureLevels,
					1,
					D3D11_SDK_VERSION,
					&gDx11Device->d3d11Device,
					&FeatureLevel,
					&gDx11Device->d3d11DeviceContext);

				if (SUCCEEDED(hr))
				{
					gDx11Device->dxgiAdapter = adapter;
					break;
				}
				else
				{
					adapter->Release();
					gDx11Device->dxgiOutput->Release();
				}

			}
		}
	}

	if (gDx11Device->d3d11Device && gDx11Device->d3d11DeviceContext)
	{

		DXGI_SWAP_CHAIN_DESC sd;
		yu::memset(&sd, 0, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = desc.width;
		sd.BufferDesc.Height = desc.height;
		sd.BufferDesc.Format = DXGIFormat(desc.format);
		sd.BufferDesc.RefreshRate.Numerator = 0; //this is important, if not set to zero, DXGI will complain when goto full screen
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = win.hwnd;
		sd.SampleDesc.Count = desc.sampleCount;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //allow switch desktop resolution
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		HRESULT hr = gDx11Device->dxgiFactory1->CreateSwapChain(gDx11Device->d3d11Device, &sd, &gDx11Device->dxgiSwapChain);

		if (SUCCEEDED(hr))
		{
			if (desc.fullScreen)
			{
				SetFullScreen(desc);
			}

			return true;
		}

		Log("error: failed to create swap chain\n");
		return false;
	}

	Log("error: failed to create d3d11 device\n");
	return false;
}

void FreeDX11()
{
	OutputDebugStringA("free dx11\n");
	ULONG refCount;
	
	refCount = gDx11Device->dxgiOutput->Release();
	refCount = gDx11Device->dxgiSwapChain->Release();
	refCount = gDx11Device->dxgiAdapter->Release();
	refCount = gDx11Device->d3d11DeviceContext->Release();
	refCount = gDx11Device->d3d11Device->Release();

	refCount = gDx11Device->dxgiFactory1->Release();

	delete gDx11Device;
}

void ResizeBackBuffer(unsigned int width, unsigned int height, TexFormat fmt)
{
	if (!gDx11Device)
		return;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::RESIZE_BUFFER;
	cmd.resizeBuffCmd.width = width;
	cmd.resizeBuffCmd.height = height;
	cmd.resizeBuffCmd.fmt = fmt;

//	if(gDx11Device->goingFullScreen)
	if (!gDx11Device->renderThreadCmdQueue.Enqueue(cmd))
	{
		Log("command queue full, ResizeBackBuffer failed\n");
	}
	
}

void ExecResizeBackBufferCmd(unsigned int width, unsigned int height, TexFormat fmt)
{

	ID3D10Texture2D* pTex;
	UINT bufIndex = 0;
	if (SUCCEEDED(gDx11Device->dxgiSwapChain->GetBuffer(bufIndex, __uuidof(pTex), reinterpret_cast<void**>(&pTex))))
	{
		UINT refCount = pTex->Release();
	}

	HRESULT hr = gDx11Device->dxgiSwapChain->ResizeBuffers(1, width, height, DXGIFormat(fmt), DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	if (FAILED(hr))
	{
		Log("error: resize buffer failed\n");
	}

}

static void ExecSwapFrameBufferCmd(bool vsync)
{
	gDx11Device->dxgiSwapChain->Present(vsync ? 1 : 0, 0);
}

ID3DBlob* CompileShaderDx11(const char* shaderSource, size_t sourceLen, const char* entryPoint, const char* profile);
ID3DBlob* CompileShaderFromFileDx11(const char* path, const char* entryPoint, const char* profile);

ID3D11InputLayout* CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* desc, UINT numElem)
{
	char shaderBuf[4096];
	StringBuilder shaderStr(shaderBuf, 4096);
	shaderStr.Cat("struct VertexInput {\n");

	shaderStr.Cat("float3 position : POSITION;\n");
	shaderStr.Cat("float4 color : COLOR;\n");

	shaderStr.Cat("};\n");
	shaderStr.Cat("struct VS_OUTPUT {\n\
				  				  	float4 Position : SV_POSITION; \n\
														};\n\
																		");
	shaderStr.Cat("VS_OUTPUT VS(VertexInput IN) {\n");
	shaderStr.Cat("VS_OUTPUT Out; \n\
				  					Out.Position = float4(0.0, 0.0, 0.0, 0.0); \n\
														return Out; \n\
																		  ");
	shaderStr.Cat("}");

	DxResourcePtr<ID3DBlob> vsShader = CompileShaderDx11(shaderStr.strBuf, shaderStr.strLen, "VS", "vs_5_0");

	ID3D11InputLayout* inputLayout;
	HRESULT hr = gDx11Device->d3d11Device->CreateInputLayout(desc, numElem, vsShader->GetBufferPointer(), vsShader->GetBufferSize(), &inputLayout);
	return inputLayout;
}

static void ExecUpdateMeshCmd(RendererDx11* renderer, MeshHandle handle)
{
	MeshData* data = renderer->meshList.Get(handle.id);
	MeshDataDx11* dx11Data = &renderer->dx11MeshList[handle.id];

	//TODO: hack, fixme
	D3D11_INPUT_ELEMENT_DESC vertDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA }
	};

	ID3D11InputLayout* inputLayout = CreateInputLayout(vertDesc, 2);
	dx11Data->inputLayout = inputLayout;

	UINT vertexSize = 0;
	if (HasPos(data->channelMask))
	{
		vertexSize += sizeof(Vector3);
	}
	if (HasTexcoord(data->channelMask))
	{
		vertexSize += sizeof(Vector2);
	}
	if (HasColor(data->channelMask))
	{
		vertexSize += sizeof(Color);
	}

	UINT vbSize = vertexSize * data->numVertices;
	void* vb = gDefaultAllocator->Alloc(vbSize);

	u8* vertex = (u8*) vb;
	for (u32 i = 0; i < data->numVertices; i++)
	{
		if (HasPos(data->channelMask))
		{
			*((Vector3*)vertex) = data->posList[i];
			vertex += sizeof(Vector3);
		}
		if (HasTexcoord(data->channelMask))
		{
			*((Vector2*)vertex) = data->texcoordList[i];
			vertex += sizeof(Vector2);
		}
		if (HasColor(data->channelMask))
		{
			*((Color*)vertex) = data->colorList[i];
			vertex += sizeof(Color);
		}
	}
	assert(vertex == (u8*)vb + vbSize);

	D3D11_BUFFER_DESC vbdesc = {};
	vbdesc.ByteWidth = vbSize;
	vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	vertexBufferData.pSysMem = vb;
	vertexBufferData.SysMemPitch = vbSize;
	vertexBufferData.SysMemSlicePitch = 0;
	ID3D11Buffer* vertexBuffer;
	HRESULT hr = gDx11Device->d3d11Device->CreateBuffer(&vbdesc, &vertexBufferData, &vertexBuffer);
	gDefaultAllocator->Free(vb);
	if (SUCCEEDED(hr))
	{
		dx11Data->vertexBuffer = vertexBuffer;
		Log("successfully created vertex buffer\n");
	}
	else
	{
		Log("error: failed to create vertex buffer\n");
	}

	UINT ibSize = data->numIndices * sizeof(u32);

	D3D11_BUFFER_DESC ibdesc = {};
	ibdesc.ByteWidth = ibSize;
	ibdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	indexBufferData.pSysMem = data->indices;
	indexBufferData.SysMemPitch = ibSize;
	indexBufferData.SysMemSlicePitch = 0;

	ID3D11Buffer* indexBuffer;
	hr = gDx11Device->d3d11Device->CreateBuffer(&ibdesc, &indexBufferData, &indexBuffer);
	if (SUCCEEDED(hr))
	{
		dx11Data->indexBuffer = indexBuffer;
		Log("successfully created index buffer\n");
	}
	else
	{
		Log("error: failed to create index buffer\n");
	}
}

static void ExecUpdateCameraCmd(RendererDx11* renderer, CameraHandle handle, const CameraData& updateData)
{
	renderer->cameraList.Get(handle.id)->UpdateData(updateData);
	CameraData* data = &renderer->cameraList.Get(handle.id)->GetMutable();
	CameraDataDx11* dx11Data = &renderer->dx11CameraList[handle.id];
	CameraConstant camConstant;
	camConstant.viewMatrix = data->ViewMatrix();
	camConstant.projectionMatrix = data->PerspectiveMatrix();

	if (!dx11Data->constantBuffer)
	{
		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.ByteWidth = sizeof(CameraConstant);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		ID3D11Buffer* constantBuffer;
		HRESULT hr = gDx11Device->d3d11Device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
		if (SUCCEEDED(hr))
		{
			dx11Data->constantBuffer = constantBuffer;
			Log("successfully created camera constant buffer\n");
		}
	}
	if (!dx11Data->constantBuffer)
	{
		Log("error: failed to create camera constant buffer\n");
		return;
	}
	D3D11_MAPPED_SUBRESOURCE mappedBuf;
	HRESULT hr = gDx11Device->d3d11DeviceContext->Map(dx11Data->constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuf);
	
	if (SUCCEEDED(hr))
	{
		*((CameraConstant*)mappedBuf.pData) = camConstant;
	}
	else
	{
		Log("error: failed to map camera constant buffer\n");
	}

	gDx11Device->d3d11DeviceContext->Unmap(dx11Data->constantBuffer, 0);
	ID3D11Buffer* constantBuffer = dx11Data->constantBuffer;
	gDx11Device->d3d11DeviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
}

static void ExecCreateVertexShaderCmd(RendererDx11* renderer, VertexShaderHandle handle, VertexShaderAPIData& data)
{
	if (data.blob)
	{
		ID3D11VertexShader* vertexShader;
		HRESULT hr = gDx11Device->d3d11Device->CreateVertexShader(data.blob->GetBufferPointer(), data.blob->GetBufferSize(), nullptr, &vertexShader);

		if (SUCCEEDED(hr))
		{
			renderer->dx11VertexShaderList[handle.id].shader = vertexShader;
#if defined YU_DEBUG
			Log("create vertex shader: %s succeeded\n", data.sourcePath.str);
#endif
		}
		else
		{
#if defined YU_DEBUG
			Log("error, create vertex shader: %s failed\n", data.sourcePath.str);
#endif
		}
		data.blob->Release();
		data.blob = nullptr;
	}
	else
	{
#if defined YU_DEBUG
		Log("error, empty blob, create vertex shader: %s failed\n", data.sourcePath.str);
#endif
	}
}

static void ExecCreatePixelShaderCmd(RendererDx11* renderer, PixelShaderHandle handle, PixelShaderAPIData& data)
{
	if (data.blob)
	{
		ID3D11PixelShader* pixelShader;
		HRESULT hr = gDx11Device->d3d11Device->CreatePixelShader(data.blob->GetBufferPointer(), data.blob->GetBufferSize(), nullptr, &pixelShader);

		if (SUCCEEDED(hr))
		{
			renderer->dx11PixelShaderList[handle.id].shader = pixelShader;
#if defined YU_DEBUG
			Log("create pixel shader: %s succeeded\n", data.sourcePath.str);
#endif
		}
		else
		{
#if defined YU_DEBUG
			Log("error, create pixel shader: %s failed\n", data.sourcePath.str);
#endif
		}
		data.blob->Release();
		data.blob = nullptr;
	}
	else
	{
#if defined YU_DEBUG
		Log("error, empty blob, create pixel shader: %s failed\n", data.sourcePath.str);
#endif
	}
}

static void ExecCreatePipelineCmd(RendererDx11* renderer, PipelineHandle handle, PipelineData& data)
{
	PipelineDx11& pipeline = renderer->dx11PipelineList[handle.id];

	bool success = true;
	if (renderer->dx11VertexShaderList[data.vs.id].shader)
	{
		pipeline.vs = renderer->dx11VertexShaderList[data.vs.id].shader;
	}
	else
	{
		Log("error, create pipeline failed, vertex shader is invalid\n");
		success = false;
	}

	if (renderer->dx11PixelShaderList[data.ps.id].shader)
	{
		pipeline.ps = renderer->dx11PixelShaderList[data.ps.id].shader;
	}
	else
	{
		Log("error, create pipeline failed, pixel shader is invalid\n");
		success = false;
	}

	if (success)
	{
		Log("create pipeline succeeded\n");
	}
}


struct VertexP3C4
{
	Vector3 pos;
	u8		color[4];
};

static void ExecRenderCmd(RendererDx11* renderer, RenderQueue* queue, int renderListIdx)
{
	RenderList* list = &(queue->renderList[renderListIdx]);
	assert(list->renderInProgress.load(std::memory_order_acquire));


	for (int i = 0; i < list->cmdCount; i++)
	{
		MeshHandle handle = list->cmd[i].mesh;
		CameraHandle camHandle = list->cmd[i].cam;
		PipelineHandle pipelineHandle = list->cmd[i].pipeline;
		CameraData* data = &renderer->cameraList.Get(camHandle.id)->GetMutable();

		gDx11Device->d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		MeshDataDx11* mesh = &renderer->dx11MeshList[handle.id];
		CameraDataDx11* cam = &renderer->dx11CameraList[camHandle.id];
		PipelineDx11* pipeline = &renderer->dx11PipelineList[pipelineHandle.id];

		gDx11Device->d3d11DeviceContext->VSSetShader(pipeline->vs, nullptr, 0);
		gDx11Device->d3d11DeviceContext->PSSetShader(pipeline->ps, nullptr, 0);

		ID3D11Buffer* constantBuffer = cam->constantBuffer;
		gDx11Device->d3d11DeviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

		UINT stride = sizeof(VertexP3C4);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffer = mesh->vertexBuffer;
		gDx11Device->d3d11DeviceContext->IASetInputLayout(mesh->inputLayout);
		gDx11Device->d3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		gDx11Device->d3d11DeviceContext->IASetIndexBuffer(mesh->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		gDx11Device->d3d11DeviceContext->DrawIndexed(list->cmd[i].numIndex, 0, 0);
	}

	//gDx11Device->d3d11DeviceContext->Flush();
	list->cmdCount = 0;
	list->renderInProgress.store(false, std::memory_order_release);
}

static void ExecThreadCmd()
{
	RenderThreadCmd cmd;
	while (gDx11Device->renderThreadCmdQueue.Dequeue(cmd))
	{
		switch (cmd.type)
		{
		case(RenderThreadCmd::RESIZE_BUFFER):
			{
				RenderThreadCmd::ResizeBufferCmd resizeCmd = cmd.resizeBuffCmd;
				ExecResizeBackBufferCmd(resizeCmd.width, resizeCmd.height, resizeCmd.fmt);
			}break;

		}
	}
}

static bool ExecThreadCmd(RendererDx11* renderer)
{
	bool frameEnd = false;
	for (int i = 0; i < renderer->numQueue; i++)
	{
		RenderQueue* queue = renderer->renderQueue[i];
		RenderThreadCmd cmd;
		while (queue->cmdList.Dequeue(cmd))
		{
			switch (cmd.type)
			{
				case(RenderThreadCmd::RESIZE_BUFFER) :
				{
					RenderThreadCmd::ResizeBufferCmd resizeCmd = cmd.resizeBuffCmd;
					ExecResizeBackBufferCmd(resizeCmd.width, resizeCmd.height, resizeCmd.fmt);
				}break;
				case (RenderThreadCmd::UPDATE_MESH) :
				{
					ExecUpdateMeshCmd(renderer, cmd.updateMeshCmd.handle);
				}break;
				case (RenderThreadCmd::UPDATE_CAMERA) :
				{
					ExecUpdateCameraCmd(renderer, cmd.updateCameraCmd.handle, cmd.updateCameraCmd.camData);
				}break;
				case(RenderThreadCmd::RENDER) :
				{
					ExecRenderCmd(renderer, queue, cmd.renderCmd.renderListIndex);
				}break;
				case (RenderThreadCmd::SWAP) :
				{
					frameEnd = true;
					ExecSwapFrameBufferCmd(cmd.swapCmd.vsync);
				}break;
				case (RenderThreadCmd::CREATE_VERTEX_SHADER) :
				{
					ExecCreateVertexShaderCmd(renderer, cmd.createVSCmd.handle, cmd.createVSCmd.data);
				}break;
				case (RenderThreadCmd::CREATE_PIXEL_SHADER) :
				{
					ExecCreatePixelShaderCmd(renderer, cmd.createPSCmd.handle, cmd.createPSCmd.data);
				}break;
				case (RenderThreadCmd::CREATE_PIPELINE) :
				{
					ExecCreatePipelineCmd(renderer, cmd.createPipelineCmd.handle, cmd.createPipelineCmd.data);
				}break;
				default:
				{
					assert(0);
				}
			}
		}
	}

	return frameEnd;
}

bool YuRunning();
void WaitForKick(FrameLock* lock);
ThreadReturn ThreadCall RenderThread(ThreadContext context)
{
	InitDX11Param* param = (InitDX11Param*)context;
	param->initDx11CS.Lock();
	InitDX11(param->win, param->desc);
	param->initDx11CS.Unlock();
	NotifyCondVar(param->initDx11CV);


	ID3D11Texture2D* backBufferTexture;
	HRESULT hr = gDx11Device->dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture);

	ID3D11RenderTargetView* renderTarget;
	hr = gDx11Device->d3d11Device->CreateRenderTargetView(backBufferTexture, NULL, &renderTarget);
	
	if (SUCCEEDED(hr))
	{
		Log("successfully created render target\n");
	}

	FrameLock* frameLock = AddFrameLock(); //TODO: seperate render thread kick from others, shoot and forget	
	u64 frameCount = 0;
	unsigned int laps = 100;
	unsigned int f = 0;
	while (YuRunning())
	{
		PerfTimer innerTimer;
		PerfTimer kTimer;

		double waitKickTime = 0;
		double completeFrameTime = 0;

		kTimer.Start();
		WaitForKick(frameLock);
		kTimer.Finish();
		waitKickTime = kTimer.DurationInMs();


		innerTimer.Start();

		float clearColor[4] = {0.7f, 0.7f, 0.7f, 0.7f};
		gDx11Device->d3d11DeviceContext->ClearRenderTargetView(renderTarget, clearColor);
		gDx11Device->d3d11DeviceContext->OMSetRenderTargets(1, &renderTarget, nullptr);
		
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)gDx11Device->frameBufferDesc.width;
		viewport.Height = (float)gDx11Device->frameBufferDesc.height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;
		gDx11Device->d3d11DeviceContext->RSSetViewports(1, &viewport);


		ExecThreadCmd();

		bool frameEnd = false;
		while (!frameEnd && YuRunning())
			frameEnd =ExecThreadCmd(gRenderer);

		BaseDoubleBufferData::SwapDirty();

		innerTimer.Finish();
		FrameComplete(frameLock);

		f++;
		/*
		if (f > laps || frameTimer.DurationInMs() > 20)
		{
			Log("render thread frame:\n");
			Log("frame time: %lf\n", frameTimer.DurationInMs());
			Log("work time: %lf\n", innerTimer.DurationInMs());
			Log("wait kick time: %lf\n", waitKickTime);
			Log("frame complete: %lf\n", completeFrameTime);
			Log("\n\n");
			f = 0;
		}
		*/
		frameCount++;

	}
	
	SAFE_RELEASE(backBufferTexture);
	SAFE_RELEASE(renderTarget);

	FreeDX11();
	return 0;
}

void InitRenderThread(const Window& win, const FrameBufferDesc& desc)
{
	InitDX11Param param;
	param.initDx11CS.Lock();
	param.win = win;
	param.desc = desc;
	renderThread = CreateThread(RenderThread, &param);
	SetThreadName(renderThread.handle, "Render Thread");
	SetThreadAffinity(renderThread.handle, 1 << 7);
	WaitForCondVar(param.initDx11CV, param.initDx11CS);
	param.initDx11CS.Unlock();
}



}