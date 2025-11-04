#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;

uniform vec2 offset;
out vec2 TexCoord;

void main()
{ 
	gl_Position = vec4(aPos.x + offset.x, aPos.y + offset.y, aPos.z, 1.0);
	TexCoord = aTexCoord;
}