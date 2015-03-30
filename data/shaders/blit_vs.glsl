#version 330 core

uniform CameraConstant
{
	mat4x4 viewMatrix;
	mat4x4 projectionMatrix;
};

layout (location = 0) in vec3 position ;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec4 color ;

out VS_OUTPUT
{
	vec4 position ;
	vec2 texcoord;
} Out;

void main(void)
{
	Out.texcoord = vec2(texcoord.x, 1-texcoord.y);
	
	gl_Position = Out.position = vec4(position, 1);
}

																																																																																		