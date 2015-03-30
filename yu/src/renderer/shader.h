#ifndef YU_SHADER_H
#define YU_SHADER_H

#if defined YU_DX11
#	include "shader_dx11.h"
#elif defined YU_GL
#	include "shader_gl.h"
#endif

namespace yu
{
VertexShaderAPIData LoadVSFromFile(const char* path);
PixelShaderAPIData	LoadPSFromFile(const char* path);
}

#endif