#include <iostream>
#include <string>

#include "Window/Window.h"
#include "Renderer/Renderer.h"
#include "Model/Model.h"
#include "Shader/Shader.h"
#include "Texture/Texture.h"

#include "../src/vendor/glm/glm.hpp"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    Window window("Vjezba3", SCR_WIDTH, SCR_HEIGHT);

    Model model("res/models/rectangle.obj");
    Shader shader("res/shaders/vShader.glsl", "res/shaders/fShader.glsl");
    Texture tex("res/textures/container.jpg");

    Renderer render;

    int offset_location = glGetUniformLocation(shader.GetID(), "offset");
    float x_offset = 0.0;
    float y_offset = 0.0;


    //go to /res/shaders/vShader.glsl to apply
    int color_location = glGetUniformLocation(shader.GetID(), "color");
    float r = 0.0;
    float g = 0.0;
    float b = 0.0;
    //black

    while (!window.isClosed())
    {
        window.ProcessInput();
        render.Clear();

        //Send Offset
        glUniform2f(offset_location, x_offset, y_offset);
        //Send Color
        glUniform3f(color_location, r, g, b);
        model.Draw(shader, tex);

        window.SwapAndPoll();
    }

    window.CloseWindow();

    return 0;


}