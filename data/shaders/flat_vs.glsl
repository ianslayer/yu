#version 330 core

uniform CameraConstant
{
	mat4x4 viewMatrix;
	mat4x4 projectionMatrix;
};

layout (location = 0) in vec3 position ;
layout (location = 1) in vec4 color ;

out VS_OUTPUT
{
	vec4 position ;
} Out;

void main(void)
{
	mat4x4 viewProjMatrix = projectionMatrix * viewMatrix;
	gl_Position = Out.position = viewProjMatrix * vec4(position, 1);
}

																																																																																		