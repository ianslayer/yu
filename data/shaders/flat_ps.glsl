#version 330 core

in PS_INPUT
{
	vec4 position;
};

layout (location = 0) out vec4 color;

uniform sampler2D colorTexture ;

void main(void)
{
	color = texture(colorTexture, vec2(1.0, 1.0));
}
