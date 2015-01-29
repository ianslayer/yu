#ifndef YU_SHADER_GL_H
#define YU_SHADER_GL_H

#include "gl_utility.h"

namespace yu
{

struct VertexShaderAPIData : public VertexShaderData
{
	GLchar*		shaderSource;
	GLint		shaderSize;
};

struct PixelShaderAPIData : public PixelShaderData
{
	GLchar*		shaderSource;
	GLint		shaderSize;
};


}

#endif
