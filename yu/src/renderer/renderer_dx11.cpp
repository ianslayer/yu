#include <d3d11.h>
#include <stdio.h>

typedef HRESULT     (WINAPI * LPCREATEDXGIFACTORY)(REFIID, void ** );
typedef HRESULT     (WINAPI * LPD3D11CREATEDEVICE)( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );

namespace yu
{
HMODULE hModuleDX11;
HMODULE hModuleDXGI;

IDXGIFactory1* dxgiFactory1;

LPCREATEDXGIFACTORY                  dllCreateDXGIFactory1;
LPD3D11CREATEDEVICE                  dllD3D11CreateDevice;

bool InitDX11()
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
	
	IDXGIAdapter1* adapter = NULL;
	for(UINT i = 0; dxgiFactory1->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);

		printf("adapter desc: %ls\n", adapterDesc.Description);
	}
	

	return true;
}

}