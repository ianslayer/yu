#include "renderer.h"
#if defined YU_DX11

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

struct Texture2DDx11
{
	DxResourcePtr<ID3D11Texture2D> texture;
	DxResourcePtr<ID3D11ShaderResourceView> textureSRV;
};

struct RenderTextureDx11
{
	DxResourcePtr<ID3D11RenderTargetView> renderTargetView;
};

struct SamplerDx11
{
	DxResourcePtr<ID3D11SamplerState> sampler;
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
	Texture2DDx11		dx11TextureList[MAX_TEXTURE];
	RenderTextureDx11	dx11RenderTextureList[MAX_TEXTURE];
	SamplerDx11			dx11SamplerList[MAX_SAMPLER];
	UINT prevNumPSSRV = 0;
};

YU_GLOBAL RendererDx11* gRenderer;

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

YU_GLOBAL RenderDeviceDx11* gDx11Device;

YU_INTERNAL DXGI_FORMAT DXGIFormat(TextureFormat fmt)
{
	switch(fmt)
	{
		case TEX_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEX_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
	Log("error, DXGIFormat: unknown format\n");
	return DXGI_FORMAT_UNKNOWN;
}

YU_INTERNAL TextureFormat TextureFormatFromDxgi(DXGI_FORMAT fmt)
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

YU_INTERNAL bool SetFullScreen(const FrameBufferDesc& desc)
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

YU_INTERNAL bool InitDX11(const Window& win, const FrameBufferDesc& desc)
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
					#if defined (YU_DEBUG)
						D3D11_CREATE_DEVICE_DEBUG,
					#else
						0,
					#endif
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

YU_INTERNAL void FreeDX11()
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

void ResizeBackBuffer(unsigned int width, unsigned int height, TextureFormat fmt)
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

YU_INTERNAL void ExecResizeBackBufferCmd(unsigned int width, unsigned int height, TextureFormat fmt)
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

YU_INTERNAL void ExecSwapFrameBufferCmd(bool vsync)
{
	gDx11Device->dxgiSwapChain->Present(vsync ? 1 : 0, 0);
}

ID3DBlob* CompileShaderDx11(const char* shaderSource, size_t sourceLen, const char* entryPoint, const char* profile);

YU_INTERNAL ID3D11InputLayout* CreateInputLayout(u32 vertChannelMask)
{
	D3D11_INPUT_ELEMENT_DESC preDefineDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA }
	};

	D3D11_INPUT_ELEMENT_DESC translateDesc[3];
	UINT numChannel = 0;
	char shaderBuf[4096];
	StringBuilder shaderStr(shaderBuf, 4096);
	shaderStr.Cat("struct VertexInput {\n");

	if (vertChannelMask & MeshData::Channel::POSITION)
	{
		translateDesc[numChannel] = preDefineDesc[0];
		shaderStr.Cat("float3 position : POSITION;\n");
		numChannel++;
	}
	if (vertChannelMask & MeshData::Channel::TEXCOORD)
	{
		translateDesc[numChannel] = preDefineDesc[1];
		shaderStr.Cat("float2 texcoord : TEXCOORD0;\n");
		numChannel++;
	}
	if (vertChannelMask & MeshData::Channel::COLOR)
	{
		translateDesc[numChannel] = preDefineDesc[2];
		shaderStr.Cat("float4 color : COLOR;\n");
		numChannel++;
	}

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
	HRESULT hr = gDx11Device->d3d11Device->CreateInputLayout(translateDesc, numChannel, vsShader->GetBufferPointer(), vsShader->GetBufferSize(), &inputLayout);
	return inputLayout;
}

YU_INTERNAL void ExecCreateMeshCmd(RendererDx11* renderer, MeshHandle mesh, u32 numVertices, u32 numIndices, u32 vertChannelMask, MeshData* meshData)
{
	UINT vertexSize = VertexSize(vertChannelMask);
	UINT vbSize = vertexSize * numVertices;
	UINT ibSize = numIndices * sizeof(u32);
	MeshDataDx11* dx11MeshData = &renderer->dx11MeshList[mesh.id];
	dx11MeshData->inputLayout = CreateInputLayout(vertChannelMask);

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = vbSize;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = ibSize;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DYNAMIC;
	ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA* initVertexDataPtr = nullptr;
	D3D11_SUBRESOURCE_DATA  initVertexData = {};
	D3D11_SUBRESOURCE_DATA* initIndexDataPtr = nullptr;
	D3D11_SUBRESOURCE_DATA  initIndexData = {};

	void* vertexbuffer = nullptr;
	void* indexBuffer = nullptr;
	if (meshData)
	{
		initVertexDataPtr = &initVertexData;
		vertexbuffer = gDefaultAllocator->Alloc(vbSize);

		u8* vertex = (u8*)vertexbuffer;
		for (u32 i = 0; i < numVertices; i++)
		{
			if (HasPos(vertChannelMask))
			{
				*((Vector3*)vertex) = meshData->posList[i];
				vertex += sizeof(Vector3);
			}
			if (HasTexcoord(vertChannelMask))
			{
				*((Vector2*)vertex) = meshData->texcoordList[i];
				vertex += sizeof(Vector2);
			}
			if (HasColor(vertChannelMask))
			{
				*((Color*)vertex) = meshData->colorList[i];
				vertex += sizeof(Color);
			}
		}
		initVertexData.pSysMem = vertexbuffer;
		initVertexData.SysMemPitch = vbSize;
		initVertexData.SysMemSlicePitch = 0;

		initIndexDataPtr = &initIndexData;
		initIndexData.pSysMem = meshData->indices;
		initIndexData.SysMemPitch = ibSize;
		initIndexData.SysMemSlicePitch = 0;
	}

	ID3D11Buffer* dx11VertexBuffer;
	HRESULT hr = gDx11Device->d3d11Device->CreateBuffer(&vbDesc, initVertexDataPtr, &dx11VertexBuffer);
	gDefaultAllocator->Free(vertexbuffer);
	if (SUCCEEDED(hr))
	{
		dx11MeshData->vertexBuffer = dx11VertexBuffer;
		Log("successfully created vertex buffer\n");
	}
	else
	{
		Log("error: failed to create vertex buffer\n");
	}


	ID3D11Buffer* dx11IndexBuffer;
	hr = gDx11Device->d3d11Device->CreateBuffer(&ibDesc, initIndexDataPtr, &dx11IndexBuffer);
	if (SUCCEEDED(hr))
	{
		dx11MeshData->indexBuffer = dx11IndexBuffer;
		Log("successfully created index buffer\n");
	}
	else
	{
		Log("error: failed to create index buffer\n");
	}

}

YU_INTERNAL void ExecUpdateMeshCmd(RendererDx11* renderer, MeshHandle mesh, u32 startVert, u32 startIndex, MeshData* meshData)
{
	//TODO: correctlly handle startVert & startIndex 
	MeshDataDx11* dx11Data = &renderer->dx11MeshList[mesh.id];

	ID3D11Buffer* dx11VertexBuffer = dx11Data->vertexBuffer;
	ID3D11Buffer* dx11IndexBuffer = dx11Data->indexBuffer;
	D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
	D3D11_MAPPED_SUBRESOURCE mappedIndexBuffer;
	HRESULT hr = gDx11Device->d3d11DeviceContext->Map(dx11VertexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedVertexBuffer);
	if (FAILED(hr))
	{
		Log("error: ExecUpdateMeshCmd dx11 map vertex buffer failed");
		gDx11Device->d3d11DeviceContext->Unmap(dx11VertexBuffer, 0);
		return;
	}
	hr = gDx11Device->d3d11DeviceContext->Map(dx11IndexBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedIndexBuffer);
	if (FAILED(hr))
	{
		Log("error: ExecUpdateMeshCmd dx11 map index buffer failed");
		gDx11Device->d3d11DeviceContext->Unmap(dx11IndexBuffer, 0);
		return;
	}

	UINT vertexSize = VertexSize(meshData->channelMask);
	UINT vbSize = vertexSize * meshData->numVertices;
	void* vb = mappedVertexBuffer.pData;

	u8* vertex = (u8*)vb;
	for (u32 i = 0; i < meshData->numVertices; i++)
	{
		if (HasPos(meshData->channelMask))
		{
			*((Vector3*)vertex) = meshData->posList[i];
			vertex += sizeof(Vector3);
		}
		if (HasTexcoord(meshData->channelMask))
		{
			*((Vector2*)vertex) = meshData->texcoordList[i];
			vertex += sizeof(Vector2);
		}
		if (HasColor(meshData->channelMask))
		{
			*((Color*)vertex) = meshData->colorList[i];
			vertex += sizeof(Color);
		}
	}
	assert(vertex == (u8*)vb + vbSize);

	memcpy(mappedIndexBuffer.pData, meshData->indices, sizeof(u32) * meshData->numIndices);

	gDx11Device->d3d11DeviceContext->Unmap(dx11VertexBuffer, 0);
	gDx11Device->d3d11DeviceContext->Unmap(dx11IndexBuffer, 0);
}

YU_INTERNAL void ExecUpdateCameraCmd(RendererDx11* renderer, CameraHandle camera, const CameraData& updateData)
{
	renderer->cameraList.Get(camera.id)->UpdateData(updateData);
	CameraData* data = &renderer->cameraList.Get(camera.id)->GetMutable();
	CameraDataDx11* dx11Data = &renderer->dx11CameraList[camera.id];
	CameraConstant camConstant;
	camConstant.viewMatrix = data->ViewMatrix();
	camConstant.projectionMatrix = data->PerspectiveMatrixDx();

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

//TODO: remove shader blob delete from this, memory should be managed by caller
YU_INTERNAL void ExecCreateVertexShaderCmd(RendererDx11* renderer, VertexShaderHandle vs, VertexShaderAPIData& data)
{
	if (data.blob)
	{
		ID3D11VertexShader* vertexShader;
		HRESULT hr = gDx11Device->d3d11Device->CreateVertexShader(data.blob->GetBufferPointer(), data.blob->GetBufferSize(), nullptr, &vertexShader);

		if (SUCCEEDED(hr))
		{
			renderer->dx11VertexShaderList[vs.id].shader = vertexShader;
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

YU_INTERNAL void ExecCreatePixelShaderCmd(RendererDx11* renderer, PixelShaderHandle ps, PixelShaderAPIData& data)
{
	if (data.blob)
	{
		ID3D11PixelShader* pixelShader;
		HRESULT hr = gDx11Device->d3d11Device->CreatePixelShader(data.blob->GetBufferPointer(), data.blob->GetBufferSize(), nullptr, &pixelShader);

		if (SUCCEEDED(hr))
		{
			renderer->dx11PixelShaderList[ps.id].shader = pixelShader;
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

YU_INTERNAL void ExecCreatePipelineCmd(RendererDx11* renderer, PipelineHandle pipeline, const PipelineData& data)
{
	PipelineDx11& dx11Pipeline = renderer->dx11PipelineList[pipeline.id];

	bool success = true;
	if (renderer->dx11VertexShaderList[data.vs.id].shader)
	{
		dx11Pipeline.vs = renderer->dx11VertexShaderList[data.vs.id].shader;
	}
	else
	{
		Log("error, ExecCreatePipelineCmd: create pipeline failed, vertex shader is invalid\n");
		success = false;
	}

	if (renderer->dx11PixelShaderList[data.ps.id].shader)
	{
		dx11Pipeline.ps = renderer->dx11PixelShaderList[data.ps.id].shader;
	}
	else
	{
		Log("error, ExecCreatePipelineCmd: create pipeline failed, pixel shader is invalid\n");
		success = false;
	}

	if (success)
	{
		Log("create pipeline succeeded\n");
	}
}

YU_INTERNAL void ExecCreateTextureCmd(RendererDx11* renderer, TextureHandle texture, const TextureDesc& texDesc, const TextureMipData* initData)
{
	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (texDesc.renderTexture)
	{
		bindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	CD3D11_TEXTURE2D_DESC dx11TexDesc(DXGIFormat(texDesc.format), (UINT)texDesc.width, (UINT)texDesc.height, 1, UINT(texDesc.mipLevels),
		bindFlags);

	D3D11_SUBRESOURCE_DATA* dx11TexData = nullptr;
	D3D11_SUBRESOURCE_DATA dx11SubResourceData[16 * 6] = {};
	if (initData)
	{
		dx11TexData = dx11SubResourceData;
		for (int i = 0; i < texDesc.mipLevels; i++)
		{
			dx11SubResourceData[i].pSysMem = initData[i].texels;
			dx11SubResourceData[i].SysMemPitch = (UINT)initData[i].texDataSize;
		}
	}
	ID3D11Texture2D* createdTexture;
	HRESULT hr = gDx11Device->d3d11Device->CreateTexture2D(&dx11TexDesc, dx11TexData, &createdTexture);
	if (SUCCEEDED(hr))
	{
		Texture2DDx11& dx11Texture = renderer->dx11TextureList[texture.id];
		dx11Texture.texture = createdTexture;

		CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGIFormat(texDesc.format));
		ID3D11ShaderResourceView* createSRV;
		hr = gDx11Device->d3d11Device->CreateShaderResourceView(createdTexture, &srvDesc, &createSRV);
		if (SUCCEEDED(hr))
		{
			renderer->dx11TextureList[texture.id].textureSRV = createSRV;
		}
		else
		{
			Log("error, ExecCreateTextureCmd: dx11 create shader resource view failed: %x\n", hr);
		}

	}
	else
	{
		Log("error, ExecCreateTextureCmd: dx11 create texture failed: %x\n", hr);
	}
}

YU_INTERNAL void ExecCreateRenderTextureCmd(RendererDx11* renderer, RenderTextureHandle& renderTexture, RenderTextureDesc& desc)
{
	TextureDesc& refTextureDesc = *renderer->textureList.Get(desc.refTexture.id);
	Texture2DDx11& dx11Texture = renderer->dx11TextureList[desc.refTexture.id];

	D3D11_RENDER_TARGET_VIEW_DESC dx11RTDesc;
	dx11RTDesc.Format = DXGIFormat(refTextureDesc.format);
	dx11RTDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	dx11RTDesc.Texture2D.MipSlice = desc.mipLevel;

	ID3D11RenderTargetView* dx11RTView;
	HRESULT hr = gDx11Device->d3d11Device->CreateRenderTargetView(dx11Texture.texture, &dx11RTDesc, &dx11RTView);
	if (SUCCEEDED(hr))
	{
		renderer->dx11RenderTextureList[renderTexture.id].renderTargetView = dx11RTView;
	}
	else
	{
		Log("error, ExecCreateRenderTextureCmd: dx11 create render texture failed%x\n", hr);
	}

}

YU_INTERNAL D3D11_FILTER Dx11Filter(SamplerStateDesc::Filter filter)
{
	switch (filter)
	{
		case(SamplerStateDesc::FILTER_POINT) :
			return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case(SamplerStateDesc::FILTER_LINEAR) :
			return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case(SamplerStateDesc::FILTER_TRILINEAR) :
			return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	}
	Log("error, unknown filter type\n");
	assert(0);
	return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

YU_INTERNAL D3D11_TEXTURE_ADDRESS_MODE Dx11AddressMode(SamplerStateDesc::AddressMode mode)
{
	switch (mode)
	{
	case(SamplerStateDesc::ADDRESS_CLAMP) :
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	case(SamplerStateDesc::ADDRESS_WRAP) :
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}
	Log("error, unknown address mode\n");
	assert(0);
	return D3D11_TEXTURE_ADDRESS_CLAMP;
}

YU_INTERNAL void ExecCreateSamplerCmd(RendererDx11* renderer, SamplerHandle sampler, const SamplerStateDesc& desc)
{
	CD3D11_SAMPLER_DESC  dx11SamplerDesc(D3D11_DEFAULT);
	dx11SamplerDesc.Filter = Dx11Filter(desc.filter);
	dx11SamplerDesc.AddressU = Dx11AddressMode(desc.addressU);
	dx11SamplerDesc.AddressV = Dx11AddressMode(desc.addressV);

	ID3D11SamplerState* createdSampler;
	HRESULT hr = gDx11Device->d3d11Device->CreateSamplerState(&dx11SamplerDesc, &createdSampler);
	if (SUCCEEDED(hr))
	{
		SamplerDx11& dx11Sampler = renderer->dx11SamplerList[sampler.id];
		dx11Sampler.sampler = createdSampler;
	}
	else
	{
		Log("error, dx11 create sampler state failed\n");
	}
}


YU_INTERNAL void ExecInsertFenceCmd(RendererDx11* renderer, FenceHandle fenceHandle)
{
	Fence* fence = renderer->fenceList.Get(fenceHandle.id);
	fence->cpuExecuted = true;
}

struct VertexP3C4
{
	Vector3 pos;
	u8		color[4];
};

YU_INTERNAL void ExecRenderCmd(RendererDx11* renderer, RenderQueue* queue, int renderListIdx)
{
	RenderList* list = &(queue->renderList[renderListIdx]);
	assert(list->renderInProgress.load(std::memory_order_acquire));
	
	for (int i = 0; i < list->cmdCount; i++)
	{
		RenderCmd& cmd = list->cmd[i];
		MeshHandle& handle = cmd.mesh;
		CameraHandle& camHandle = cmd.cam;
		PipelineHandle& pipelineHandle = cmd.pipeline;
		RenderResource& resources = cmd.resources;

		//CameraData* data = &renderer->cameraList.Get(camHandle.id)->GetMutable();

		gDx11Device->d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		MeshDataDx11* mesh = &renderer->dx11MeshList[handle.id];
		CameraDataDx11* cam = &renderer->dx11CameraList[camHandle.id];
		PipelineDx11* pipeline = &renderer->dx11PipelineList[pipelineHandle.id];

		gDx11Device->d3d11DeviceContext->VSSetShader(pipeline->vs, nullptr, 0);
		gDx11Device->d3d11DeviceContext->PSSetShader(pipeline->ps, nullptr, 0);

		ID3D11Buffer* constantBuffer = cam->constantBuffer;
		gDx11Device->d3d11DeviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

		if (renderer->prevNumPSSRV > resources.numPsTexture)
		{
			UINT startDisableTex = resources.numPsTexture;
			UINT numDisableTex = renderer->prevNumPSSRV - resources.numPsTexture;
			for (UINT tex = startDisableTex; tex < numDisableTex; tex++)
			{
				ID3D11ShaderResourceView* dx11TextureSRV = nullptr;
				gDx11Device->d3d11DeviceContext->PSSetShaderResources(tex, 1, &dx11TextureSRV);
			}
		}

		for (UINT tex = 0; tex < resources.numPsTexture; tex++)
		{
			ID3D11ShaderResourceView* dx11TextureSRV = renderer->dx11TextureList[resources.psTextures[tex].textures.id].textureSRV;
			ID3D11SamplerState* dx11Sampler = renderer->dx11SamplerList[resources.psTextures[tex].sampler.id].sampler;
			gDx11Device->d3d11DeviceContext->PSSetShaderResources(tex, 1, &dx11TextureSRV);
			gDx11Device->d3d11DeviceContext->PSSetSamplers(tex, 1, &dx11Sampler);
		}
		renderer->prevNumPSSRV = resources.numPsTexture;

		UINT stride = sizeof(VertexP3C4);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffer = mesh->vertexBuffer;
		gDx11Device->d3d11DeviceContext->IASetInputLayout(mesh->inputLayout);
		gDx11Device->d3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		gDx11Device->d3d11DeviceContext->IASetIndexBuffer(mesh->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		gDx11Device->d3d11DeviceContext->DrawIndexed(list->cmd[i].numIndex, 0, 0);
	}

	//gDx11Device->d3d11DeviceContext->Flush();
	
	{
		list->cmdCount = 0;
		list->renderInProgress.store(false, std::memory_order_release);
		list->scratchBuffer.Rewind(0);
	}

}

YU_INTERNAL void ExecThreadCmd()
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

YU_INTERNAL bool ExecThreadCmd(RendererDx11* renderer)
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
				case (RenderThreadCmd::CREATE_MESH) :
				{
					ExecCreateMeshCmd(renderer, cmd.createMeshCmd.mesh, cmd.createMeshCmd.numVertices, cmd.createMeshCmd.numIndices, 
						cmd.createMeshCmd.vertChannelMask, cmd.createMeshCmd.meshData);
				}break;
				case (RenderThreadCmd::UPDATE_MESH) :
				{
					ExecUpdateMeshCmd(renderer, cmd.updateMeshCmd.mesh, cmd.updateMeshCmd.startVertex, cmd.updateMeshCmd.startIndex, cmd.updateMeshCmd.meshData);
				}break;
				case (RenderThreadCmd::UPDATE_CAMERA) :
				{
					ExecUpdateCameraCmd(renderer, cmd.updateCameraCmd.camera, cmd.updateCameraCmd.camData);
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
					ExecCreateVertexShaderCmd(renderer, cmd.createVSCmd.vertexShader, cmd.createVSCmd.data);
				}break;
				case (RenderThreadCmd::CREATE_PIXEL_SHADER) :
				{
					ExecCreatePixelShaderCmd(renderer, cmd.createPSCmd.pixelShader, cmd.createPSCmd.data);
				}break;
				case (RenderThreadCmd::CREATE_PIPELINE) :
				{
					ExecCreatePipelineCmd(renderer, cmd.createPipelineCmd.pipeline, cmd.createPipelineCmd.data);
				}break;
				case (RenderThreadCmd::CREATE_TEXTURE) :
				{
					ExecCreateTextureCmd(renderer, cmd.createTextureCmd.texture, cmd.createTextureCmd.desc, cmd.createTextureCmd.initData);
				}break;
				case (RenderThreadCmd::CREATE_RENDER_TEXTURE) :
				{
					ExecCreateRenderTextureCmd(renderer, cmd.createRenderTextureCmd.renderTexture, cmd.createRenderTextureCmd.desc);
				}break;
				case (RenderThreadCmd::CREATE_SAMPLER) :
				{
					ExecCreateSamplerCmd(renderer, cmd.createSamplerCmd.sampler, cmd.createSamplerCmd.desc);
				}break;
				case (RenderThreadCmd::CREATE_FENCE):
				{
					//nothing to do
				}break;
				case (RenderThreadCmd::INSERT_FENCE):
				{
					ExecInsertFenceCmd(renderer, cmd.insertFenceCmd.fence);
				}break;
				default:
				{
					Log("error: unknown render thread cmd\n");
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
		kTimer.Stop();
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

		innerTimer.Stop();
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

#endif