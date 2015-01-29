#include "../core/log.h"
#include "../core/file.h"
#include "shader_dx11.h"

typedef HRESULT WINAPI D3DCompileFunc( LPCVOID pSrcData,
SIZE_T SrcDataSize,
LPCSTR pSourceName,
CONST D3D_SHADER_MACRO* pDefines,
ID3DInclude* pInclude,
LPCSTR pEntrypoint,
LPCSTR pTarget,
UINT Flags1,
UINT Flags2,
ID3DBlob** ppCode,
ID3DBlob** ppErrorMsgs);

D3DCompileFunc*  dllD3DCompile;

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

YU_INTERNAL void InitDx11Compiler()
{
	unsigned int maxVersion = 47;
	unsigned int minVersion = 33;

	for (unsigned int version = maxVersion; version >= minVersion; version--)
	{
		char dllNameBuffer[20] = "D3DCompiler_";
		StringBuilder dllName(dllNameBuffer, 20, strlen(dllNameBuffer));

		dllName.Cat(version);
		dllName.Cat(".dll");

		HMODULE compilerModule = LoadLibraryA(dllName.strBuf);

		if (compilerModule)
			dllD3DCompile = (D3DCompileFunc*)GetProcAddress(compilerModule, "D3DCompile");
		else
		{
			Log("load d3d shader compiler version : %s failed\n", dllName.strBuf);
		}
		if (dllD3DCompile)
			break;

	}

}

ID3DBlob* CompileShaderDx11(const char* shaderSource, size_t sourceLen, const char* entryPoint, const char* profile)
{
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	D3DInputHandler includeHandler;

	if (!dllD3DCompile)
	{
		InitDx11Compiler();
	}
	if (!dllD3DCompile)
	{
		Log("error, can't initialize d3d11 shader compiler\n");
		exit(1);
	}
	HRESULT hr = dllD3DCompile(shaderSource, sourceLen, NULL, NULL, &includeHandler, entryPoint, profile, 0, 0, &shaderBlob, &errorBlob);

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

VertexShaderAPIData CompileVSFromFile(const char* path)
{
	VertexShaderAPIData data;
#if defined YU_DEBUG
	data.sourcePath = InternStr(path);
#endif
	data.blob =  CompileShaderFromFileDx11(path, "main", "vs_5_0");
	return data;
}

PixelShaderAPIData CompilePSFromFile(const char* path)
{
	PixelShaderAPIData data;
#if defined YU_DEBUG
	data.sourcePath = InternStr(path);
#endif
	data.blob = CompileShaderFromFileDx11(path, "main", "ps_5_0");
	return data;
}

VertexShaderAPIData LoadVSFromFile(const char* path)
{
	return CompileVSFromFile(path);
}

PixelShaderAPIData LoadPSFromFile(const char* path)
{
	return CompilePSFromFile(path);
}

}