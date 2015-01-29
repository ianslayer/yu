#ifndef YU_SHADER_H
#define YU_SHADER_H

#if defined YU_OS_WIN32
#	define YU_DX11
#	define YU_GRAPHICS_API YU_DX11
#elif defined YU_OS_MAC
#	define YU_GL
#	define YU_GRAPHICS_API YU_GL
#endif

#if defined YU_DX11
#	include "shader_dx11.h"
#else
#	include "shader_gl.h"
#endif

namespace yu
{
VertexShaderAPIData LoadVSFromFile(const char* path);
PixelShaderAPIData LoadPSFromFile(const char* path);
}

#endif