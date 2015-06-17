#version 330 core

layout(row_major) uniform CameraConstant
{
	mat4x4 viewMatrix;
	mat4x4 projectionMatrix;
};

layout(row_major) uniform  TransformConstant
{
	mat4x4 modelMatrix;
};

layout (location = 0) in vec3 position ;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec4 color ;

out VS_OUTPUT
{
	vec4 position ;
	vec4 color;
	vec2 texcoord;
} Out;

void main(void)
{
	mat4x4 viewProjMatrix = projectionMatrix * viewMatrix;
	Out.texcoord = vec2(texcoord.x, 1-texcoord.y);
	Out.color = color;
	gl_Position = Out.position = viewProjMatrix * modelMatrix* vec4(position, 1);
}

																																																																																		