#include <iostream>
#include <string>

#include "Window.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    Window window("Vjezba3", SCR_WIDTH, SCR_HEIGHT);

    Model model("res/models/rectangle.obj");
    Shader shader("res/shaders/vShader.glsl", "res/shaders/fShader.glsl");
    Texture tex("res/textures/container.jpg");

    Renderer render;

    // --- Inicijalne vrijednosti za zadatke ---
    // 1) Pomak (horizontalni, vertikalni, z)
    float offsetX = 0.2f;   // pomak u x smjeru (lijevo/desno)
    float offsetY = -0.1f;  // pomak u y smjeru (gore/dolje)
    float offsetZ = 0.0f;   // po potrebi

    // 2) Boja (RGB)
    float colorR = 1.0f; // npr. crvena
    float colorG = 1.0f; // zelena
    float colorB = 1.0f; // plava

    // 3) Koristiti teksturu ili ne (1 = yes, 0 = no)
    float useTexture = 1.0f;

    // obavezno bind shader i postavi statične/uniform vrijednosti koje se ne mijenjaju svaki frame
    shader.Bind();

    // poveži sampler uniform s texture unit 0
    shader.SetUniformInt("tex", 0);

    // petlja
    while (!window.isClosed())
    {
        window.ProcessInput();
        render.Clear();

        // update uniformi prije crtanja
        shader.Bind();
        // offset kao vec3 (koristimo postojeću funkciju SetUniformVec3 iz Shader klase)
        shader.SetUniformVec3("offset", glm::vec3(offsetX, offsetY, offsetZ));

        // boja
        shader.SetUniformVec3("uColor", glm::vec3(colorR, colorG, colorB));

        // use texture (float uniform)
        shader.SetUniformFloat("useTexture", useTexture);

        // modeli crtaju (Model::Draw interna bind/texture poziva)
        model.Draw(shader, tex);

        window.SwapAndPoll();
    }

    window.CloseWindow();

    return 0;
}
