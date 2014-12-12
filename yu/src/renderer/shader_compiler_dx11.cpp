#include "../core/log.h"
#include "../core/file.h"
#include <D3DCompiler.h>
#pragma comment (lib, "D3dcompiler.lib ")

#define SAFE_RELEASE(p) \
		{\
		ULONG testRef = 0;\
		if(p)\
			testRef = p->Release();\
		p = 0;\
		}

namespace yu
{
struct D3DInputHandler : public ID3DInclude
{
	STDMETHOD(Close)(LPCVOID pData) {

		return S_OK;
	}
	STDMETHOD(Open) (D3D_INCLUDE_TYPE IncludeType,
		LPCSTR pFileName,
		LPCVOID pParentData,
		LPCVOID *ppData,
		UINT *pBytes)
	{

		return S_OK;
	}

};

ID3DBlob* CompileShaderDx11(const char* shaderSource, size_t sourceLen, const char* entryPoint, const char* profile)
{
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	D3DInputHandler includeHandler;

	HRESULT hr = D3DCompile(shaderSource, sourceLen, NULL, NULL, &includeHandler, entryPoint, profile, 0, 0, &shaderBlob, &errorBlob);

	if (FAILED(hr))
	{
		Log("dx shader compile error:\n %s", (char*)errorBlob->GetBufferPointer());

	}
	SAFE_RELEASE(errorBlob);

	return shaderBlob;
}

ID3DBlob* CompileShaderFromFileDx11(const char* path, const char* entryPoint, const char* profile)
{
	size_t fileSize = FileSize(path);
	if (fileSize == 0)
	{
		Log("error, dx11 shader compiler can't open file:%s\n", path);
		return nullptr;
	}
	char* shaderSource = new char[fileSize + 1];
	memset(shaderSource, 0, fileSize + 1);
	size_t readSize = ReadFile(path, shaderSource, fileSize);
	ID3DBlob* shaderBlob = CompileShaderDx11(shaderSource, fileSize, entryPoint, profile);
	delete[] shaderSource;

	return shaderBlob;


}

}