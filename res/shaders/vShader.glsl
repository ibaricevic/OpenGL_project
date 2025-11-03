#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCord;

out vec3 normal;
out vec2 TexCord;

// uniform za pomak, koristimo vec3 tako da možemo lako dodati i Z ako zatreba
uniform vec3 offset;

void main()
{ 
    // pomaknemo vertex poziciju za offset (offset.xy = horizontalni i vertikalni pomak)
    vec3 movedPos = aPos + offset;
	gl_Position = vec4(movedPos, 1.0f);

	normal = aNorm;
	TexCord = aTexCord;
}