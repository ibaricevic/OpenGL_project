#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCord;

out vec3 normal;
out vec2 TexCord;

uniform vec2 offset;

void main()
{ 
	vec3 newPos=aPos+vec3(offset,0.0);
	gl_Position=vec4(newPos,1.0f);
	normal=aNorm;
	TexCord=aTexCord;
}