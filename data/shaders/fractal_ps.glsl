#version 330 core

in VS_OUTPUT
{
	vec4 position;
	vec2 texcoord;
} In;

layout (location = 0) out vec4 color;

uniform sampler2D prevIterTexture ;

void main(void)
{
	color = texture(prevIterTexture, In.texcoord);
}
