#include <math.h>
#include <d3d11.h>
#include <stdio.h>
#include "../core/system.h"
#include "renderer.h"
typedef HRESULT     (WINAPI * LPCREATEDXGIFACTORY)(REFIID, void ** );
typedef HRESULT     (WINAPI * LPD3D11CREATEDEVICE)( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );

namespace yu
{
HMODULE hModuleDX11;
HMODULE hModuleDXGI;

IDXGIFactory1* dxgiFactory1 = nullptr;
IDXGIOutput* dxgiOutput = nullptr;
IDXGISwapChain* dxgiSwapChain = nullptr;

ID3D11Device* d3d11Device = nullptr;
ID3D11DeviceContext* d3d11DeviceContext = nullptr;

LPCREATEDXGIFACTORY                  dllCreateDXGIFactory1;
LPD3D11CREATEDEVICE                  dllD3D11CreateDevice;

DXGI_FORMAT DXGIFormat(TexFormat fmt)
{
	switch(fmt)
	{
		case TEX_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEX_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}

	return DXGI_FORMAT_UNKNOWN;
}

bool InitDX11(const Window& win, const FrameBufferDesc& desc)
{
	hModuleDX11 = LoadLibrary(TEXT("d3d11.dll"));

	if(hModuleDX11 != NULL)
	{
		dllD3D11CreateDevice = ( LPD3D11CREATEDEVICE )GetProcAddress( hModuleDX11, "D3D11CreateDevice" );
		if(dllD3D11CreateDevice == NULL)
		{
			printf("error: failed to load D3D11CreateDevice\n");
			return false;
		}
	}
	else 
	{
		printf("error: can't find d3d11.dll\n");
		return false;
	}

	hModuleDXGI = LoadLibrary(TEXT("dxgi.dll"));

	if(hModuleDXGI != NULL)
	{
		dllCreateDXGIFactory1 = (LPCREATEDXGIFACTORY) GetProcAddress(hModuleDXGI, "CreateDXGIFactory1");
		if(dllCreateDXGIFactory1 ==NULL)
		{
			printf("error: failed to load CreateDXGIFactory1");
			return false;
		}
	}
	else
	{
		printf("error: can't find dxgi.dll\n");
		return false;
	}

	HRESULT hr;
	hr = dllCreateDXGIFactory1(__uuidof( IDXGIFactory1 ), ( LPVOID* ) &dxgiFactory1);

	if(FAILED(hr) )
	{
		printf("error: failed to create DXGIFactory1\n");
		return false;
	}
	
	Display mainDisplay = gSystem->GetMainDisplay();

	IDXGIAdapter1* adapter = NULL;
	for(UINT i = 0; dxgiFactory1->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);

		printf("adapter desc: %ls\n", adapterDesc.Description);
	

		for(UINT j = 0; adapter->EnumOutputs(j, &dxgiOutput) != DXGI_ERROR_NOT_FOUND; j++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			dxgiOutput->GetDesc(&outputDesc);

			UINT numDispMode = 0;
			hr = dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numDispMode, NULL);

			DXGI_MODE_DESC* pModeList = new DXGI_MODE_DESC[numDispMode];

			hr = dxgiOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numDispMode, pModeList);
			printf("output mode:\n");
			for(UINT m = 0; m < numDispMode; m++)
			{
				
				printf("width:%d, height:%d, refresh rate: %lf \n", pModeList[m].Width, pModeList[m].Height, (double)pModeList[m].RefreshRate.Numerator / (double)pModeList[m].RefreshRate.Denominator);
			}

			delete[] pModeList;

			printf("output monitor: %ls\n", outputDesc.Monitor);
			printf("output device: %ls\n", outputDesc.DeviceName);

			if(outputDesc.Monitor == mainDisplay.hMonitor)
			{
				printf("main display\n");

				D3D_FEATURE_LEVEL FeatureLevel;
				D3D_FEATURE_LEVEL FeatureLevels[1] = {D3D_FEATURE_LEVEL_11_0};

				hr  =  dllD3D11CreateDevice(adapter,
						D3D_DRIVER_TYPE_UNKNOWN, 
						//driverType, 
						0, 
						0, //desc.directXDebugInfo == true?D3D11_CREATE_DEVICE_DEBUG:0, 
						FeatureLevels,
						1,
						D3D11_SDK_VERSION, 
						&d3d11Device,
						&FeatureLevel,
						&d3d11DeviceContext);

				if(SUCCEEDED(hr))
					break;

			}
		}
	}

	if(d3d11Device && d3d11DeviceContext)
	{
	
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount = 1;
		sd.BufferDesc.Width = desc.width;
		sd.BufferDesc.Height = desc.height;
		sd.BufferDesc.Format = DXGIFormat(desc.format);
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = win.hwnd;
		sd.SampleDesc.Count = desc.sampleCount;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //allow switch desktop resolution
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		HRESULT hr = dxgiFactory1->CreateSwapChain(d3d11Device, &sd, &dxgiSwapChain);
		
		if(SUCCEEDED(hr))
		{
			if(desc.fullScreen)
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
				hr = dxgiOutput->FindClosestMatchingMode(&desiredMode, &matchMode, d3d11Device);
				if(SUCCEEDED(hr))
				{
					 hr = dxgiSwapChain->ResizeTarget(&matchMode);
					if (SUCCEEDED(hr))
					{
						HRESULT switchHr = dxgiSwapChain->SetFullscreenState(TRUE, dxgiOutput); //FIXME: should wait until switch process is done

						if(SUCCEEDED(switchHr))
						{
							printf("switch to full screen\n");
						}

					}
				}
				else
				{
					printf("failed to enter fullscreen mode\n");
				}
			}

			return true;
		}

		printf("error: failed to create swap chain\n");
		return false;
	}

	printf("error: failed to create d3d11 device\n");
	return false;
}

void SwapFrameBuffer()
{
	dxgiSwapChain->Present(0, 0);
}

void ResizeBackBuffer(unsigned int width, unsigned int height)
{
	if (dxgiSwapChain)
	{
		ID3D10Texture2D* pTex;
		UINT bufIndex = 0;
		if (SUCCEEDED(dxgiSwapChain->GetBuffer(bufIndex, __uuidof(pTex), reinterpret_cast<void**>(&pTex) ) ) )
		{
			UINT refCount = pTex->Release();
		}

		HRESULT hr = dxgiSwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		if (FAILED(hr))
		{
			printf("error: resize buffer failed\n");
		}
	}
}

}