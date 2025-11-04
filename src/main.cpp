#include <iostream>
#include <string>

#include "Window.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    Window window("Vjezba3", SCR_WIDTH, SCR_HEIGHT);

    Model model("res/models/rectangle.obj");
    Shader shader("res/shaders/vShader.glsl", "res/shaders/fShader.glsl");
    Texture tex("res/textures/HH.png");

    Renderer render;

    float offsetX = 0.3f;
    float offsetY = 0.2f;
    glm::vec3 color(0.0f, 0.0f, 0.0f);

    while (!window.isClosed())
    {
        window.ProcessInput();
        render.Clear();

        shader.Bind();

        int programID;
        glGetIntegerv(GL_CURRENT_PROGRAM, &programID);

        float timeValue = glfwGetTime();
		offsetX = sin(timeValue) * 0.5f;
		offsetY = cos(timeValue) * 0.5f;
		//color.r = (sin(timeValue) + 1.0f) / 1.0f;
		color.g = (sin(timeValue) + 1.0f) / 1.0f;

        int offsetLoc = glGetUniformLocation(programID, "offset");
        int colorLoc = glGetUniformLocation(programID, "color");
        glUniform2f(offsetLoc, offsetX, offsetY);
        glUniform3f(colorLoc, color.x, color.y, color.z);
        model.Draw(shader, tex);

        window.SwapAndPoll();
    }

    window.CloseWindow();

    return 0;
}