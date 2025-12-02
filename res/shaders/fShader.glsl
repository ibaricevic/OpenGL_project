#version 330 core

in vec3 normal;
in vec2 TexCord;

out vec4 fColor;

uniform vec3 color;
uniform sampler2D tex;

void main()
{
	vec4 texColor = texture(tex, TexCord);
	fColor= texColor * vec4(color, 1.0);
}