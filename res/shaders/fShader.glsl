#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 color;
uniform sampler2D tex;

void main()
{
	vec4 base = texture(tex, TexCoord);
	FragColor = mix(vec4(color, 1.0), base, 0.7);

}