#ifndef YU_SHADER_DX11_H
#define YU_SHADER_DX11_H

#include "renderer.h"
#include <d3dcommon.h>

namespace yu
{

struct VertexShaderAPIData : public VertexShaderData
{
	ID3DBlob* blob;
};

struct PixelShaderAPIData : public PixelShaderData
{
	ID3DBlob* blob;
};

VertexShaderAPIData CompileVSFromFile(const char* path);
PixelShaderAPIData CompilePSFromFile(const char* path);

}

#endif