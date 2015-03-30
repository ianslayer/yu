#version 330 core

in VS_OUTPUT
{
	vec4 position;
	vec2 texcoord;
} In;

layout (location = 0) out vec2 c;

void main(void)
{
	c = vec2(texcoord.x, 1.0 - texcoord.y);
}
