#include "../math/vector.h"
#include <d3d11.h>
#include "../container/dequeue.h"
#include "../core/system.h"
#include "../core/log.h"
#include "../core/thread.h"
#include "../core/timer.h"
#include "../core/string.h"
#include "renderer.h"
typedef HRESULT WINAPI  LPCREATEDXGIFACTORY(REFIID, void ** );
typedef HRESULT WINAPI  LPD3D11CREATEDEVICE(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );

namespace yu
{
HMODULE hModuleDX11;
HMODULE hModuleDXGI;
LPCREATEDXGIFACTORY*    dllCreateDXGIFactory1;
LPD3D11CREATEDEVICE*    dllD3D11CreateDevice;

Thread				renderThread;

#define SAFE_RELEASE(p) \
		{\
		ULONG testRef = 0;\
		if(p)\
			testRef = p->Release();\
		p = 0;\
		}

struct RenderThreadCmd
{
	enum CommandType
	{
		RESIZE_BUFFER,
	};

	CommandType type;

	struct ResizeBufferCmd
	{
		unsigned int	width;
		unsigned int	height;
		TexFormat		fmt;
	};

	union Command
	{
		ResizeBufferCmd resizeBuffCmd;
	} cmd;
};

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

bool SetFullScreen(const FrameBufferDesc& desc)
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

bool InitDX11(const Window& win, const FrameBufferDesc& desc)
{
	gDx11Device = new RenderDeviceDx11();

	gDx11Device->frameBufferDesc = desc;

	hModuleDX11 = LoadLibrary(TEXT("d3d11.dll"));

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

	hModuleDXGI = LoadLibrary(TEXT("dxgi.dll"));

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

	Display mainDisplay = System::GetMainDisplay();

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
	cmd.cmd.resizeBuffCmd.width = width;
	cmd.cmd.resizeBuffCmd.height = height;
	cmd.cmd.resizeBuffCmd.fmt = fmt;

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

static void ExecThreadCmd()
{
	RenderThreadCmd cmd;
	while (gDx11Device->renderThreadCmdQueue.Dequeue(cmd))
	{
		switch (cmd.type)
		{
		case(RenderThreadCmd::RESIZE_BUFFER):
			{
				RenderThreadCmd::ResizeBufferCmd resizeCmd = cmd.cmd.resizeBuffCmd;
				ExecResizeBackBufferCmd(resizeCmd.width, resizeCmd.height, resizeCmd.fmt);
			}break;
		}
	}
}

ID3DBlob* CompileShaderDx11(const char* shaderSource, size_t sourceLen, const char* entryPoint, const char* profile);

static const char* testShader = "";


struct VertexP3C4
{
	Vector3 pos;
	u8		color[4];
};

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

	ID3DBlob* vsShader = CompileShaderDx11(shaderStr.strBuf, shaderStr.strLen, "VS", "vs_5_0");

	ID3D11InputLayout* inputLayout;
	HRESULT hr = gDx11Device->d3d11Device->CreateInputLayout(desc, numElem, vsShader->GetBufferPointer(), vsShader->GetBufferSize(), &inputLayout);

	SAFE_RELEASE(vsShader);
	return inputLayout;
}

const char* vsShaderSrc = " struct VertexInput { \n\
							float3 position : POSITION; \n\
							float4 color : COLOR;\n\
						};\n\
						struct VS_OUTPUT {\n\
							float4 Position : SV_POSITION; \n\
						};\n\
						VS_OUTPUT VS(VertexInput IN) { \n\
							VS_OUTPUT Out;\n\
							Out.Position = float4(IN.position, 1); \n\
							return Out; \n\
						} \n\
";

const char* psShaderSrc = " struct VS_OUTPUT {\n\
							float4 Position : SV_POSITION; \n\
							};\n\
							struct PS_OUTPUT {\n\
						  	float4 color: SV_TARGET0; \n\
						  	};\n\
							PS_OUTPUT PS(VS_OUTPUT IN) {\n\
								PS_OUTPUT Out; \n\
								Out.color = float4(1, 0, 0, 1); \n\
								return Out; \n\
							}\
						  ";

bool YuRunning();
void WaitForKick(FrameLock* lock);
ThreadReturn ThreadCall RenderThread(ThreadContext context)
{
	InitDX11Param* param = (InitDX11Param*)context;
	param->initDx11CS.Lock();
	InitDX11(param->win, param->desc);
	param->initDx11CS.Unlock();
	NotifyCondVar(param->initDx11CV);

	FrameLock* frameLock = AddFrameLock();

	D3D11_INPUT_ELEMENT_DESC vertDesc[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA }
	};

	ID3D11InputLayout* inputLayout = CreateInputLayout(vertDesc, 2);
	ID3DBlob* vertexShaderBlob = CompileShaderDx11(vsShaderSrc, strlen(vsShaderSrc), "VS", "vs_5_0");
	ID3DBlob* pixelShaderBlob = CompileShaderDx11(psShaderSrc, strlen(psShaderSrc), "PS", "ps_5_0");

	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;

	if (vertexShaderBlob)
	{
		HRESULT hr = gDx11Device->d3d11Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader);
		if (SUCCEEDED(hr))
		{
			Log("successfully created vertex shader\n");
		}
	}

	if (pixelShaderBlob)
	{
		HRESULT hr = gDx11Device->d3d11Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader);
		if (SUCCEEDED(hr))
		{
			Log("successfully created pixl shader\n");
		}
	}

	VertexP3C4 triangle[3];
	triangle[0].pos = Vector3(0, 0.5, 0.5);
	triangle[1].pos = Vector3(0.5, -0.5, 0.5);
	triangle[2].pos = Vector3(-0.5, -0.5, 0.5);

	u32 indices[3] = { 0, 1, 2 };

	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;


	D3D11_BUFFER_DESC vbdesc = {};
	vbdesc.ByteWidth = sizeof(triangle);
	vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	vertexBufferData.pSysMem = &triangle;
	vertexBufferData.SysMemPitch = sizeof(triangle);
	vertexBufferData.SysMemSlicePitch = 0;
	HRESULT hr = gDx11Device->d3d11Device->CreateBuffer(&vbdesc, &vertexBufferData, &vertexBuffer);
	if (SUCCEEDED(hr))
	{
		Log("successfully created vertex buffer\n");
	}

	D3D11_BUFFER_DESC ibdesc = {};
	ibdesc.ByteWidth = sizeof(indices);
	ibdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	indexBufferData.pSysMem = indices;
	indexBufferData.SysMemPitch = sizeof(indices);
	indexBufferData.SysMemSlicePitch = 0;

	 hr = gDx11Device->d3d11Device->CreateBuffer(&ibdesc, &indexBufferData, &indexBuffer);
	if (SUCCEEDED(hr))
	{
		Log("successfully created index buffer\n");
	}

	ID3D11Texture2D* backBufferTexture;
	hr = gDx11Device->dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture);

	ID3D11RenderTargetView* renderTarget;
	hr = gDx11Device->d3d11Device->CreateRenderTargetView(backBufferTexture, NULL, &renderTarget);
	
	if (SUCCEEDED(hr))
	{
		Log("successfully created render target\n");
	}
	
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

		ExecThreadCmd();

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
		gDx11Device->d3d11DeviceContext->IASetInputLayout(inputLayout);
		UINT stride = sizeof(VertexP3C4);
		UINT offset = 0;
		gDx11Device->d3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		gDx11Device->d3d11DeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		gDx11Device->d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gDx11Device->d3d11DeviceContext->VSSetShader(vertexShader, nullptr, 0);
		gDx11Device->d3d11DeviceContext->PSSetShader(pixelShader, nullptr, 0);
		gDx11Device->d3d11DeviceContext->DrawIndexed(3, 0, 0);
		gDx11Device->d3d11DeviceContext->Flush();
		SwapFrameBuffer();
		innerTimer.Finish();


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
		FrameComplete(frameLock);
	}
	SAFE_RELEASE(backBufferTexture);
	SAFE_RELEASE(renderTarget);
	SAFE_RELEASE(vertexShader);
	SAFE_RELEASE(pixelShader);
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);
	SAFE_RELEASE(inputLayout);

	FreeDX11();
	return 0;
}

void InitDX11Thread(const Window& win, const FrameBufferDesc& desc)
{
	InitDX11Param param;
	param.initDx11CS.Lock();
	param.win = win;
	param.desc = desc;
	renderThread = CreateThread(RenderThread, &param);
	SetThreadName(renderThread.handle, "Render Thread");
	SetThreadAffinity(renderThread.handle,  1 << 7);
	WaitForCondVar(param.initDx11CV, param.initDx11CS);
	param.initDx11CS.Unlock();
}

void SwapFrameBuffer()
{
	gDx11Device->dxgiSwapChain->Present(0, 0);
}



}