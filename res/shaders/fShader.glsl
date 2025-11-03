#version 330 core

in vec3 normal;
in vec2 TexCord;

out vec4 fColor;

uniform sampler2D tex;    // texture sampler (0)
uniform vec3 uColor;      // boja definirana u C++ (r,g,b)
uniform float useTexture; // 1.0 = koristi teksturu, 0.0 = samo boja

void main()
{
    // dohvat boje teksture
    vec4 tcol = texture(tex, TexCord);

    // Ako je useTexture==1 koristimo teksturu i tintamo ju s uColor
    // Ako je useTexture==0, ispisujemo samo uColor
    if (useTexture >= 0.5)
        fColor = vec4(uColor, 1.0) * tcol;
    else
        fColor = vec4(uColor, 1.0);
}