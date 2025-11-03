#version 330 core

in vec3 normals;
in vec2 TexCord;

out vec4 fColor;
out vec4 rectangle_color;

uniform sampler2D tex;
uniform vec3 color;

void main()
{
	//this will change the color of the rectangle
	//rectangle_color=vec4(color,1.0);
	fColor=texture(tex, TexCord);
}