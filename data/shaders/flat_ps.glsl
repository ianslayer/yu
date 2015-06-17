#version 330 core

in VS_OUTPUT
{
	vec4 position;
	vec4 color;
	vec2 texcoord;
} In;

layout (location = 0) out vec4 color;

uniform sampler2D colorTexture ;

void main(void)
{
	color = texture(colorTexture, In.texcoord);
}
