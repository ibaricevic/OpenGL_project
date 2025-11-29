#include <iostream>
#include <string>

#include "Window.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool usePerspective = true;

int main()
{
    Window window("Vjezba4", SCR_WIDTH, SCR_HEIGHT);

    Model model("res/models/rectangle.obj");
    Shader shader("res/shaders/vShader.glsl", "res/shaders/fShader.glsl");
    Texture tex("res/textures/container.jpg");

    Renderer render;

    std::vector<glm::vec3> positions = {
        {-1.5f, 0.0f, 0.0f},
        { 0.0f, 0.0f, 0.0f},
        { 1.5f, 0.0f, 0.0f}
    };

    //glm::vec3 color(0.0f, 0.0f, 0.0f);

    while (!window.isClosed())
    {
        window.ProcessInput();
        render.Clear();

        shader.Bind();

        // ----------------------------------------------------
        // VIEW MATRIX – obična statična kamera
        // ----------------------------------------------------
        glm::mat4 view = glm::lookAt(
            glm::vec3(4.0f, 3.0f, 4.0f),   // kamera
            glm::vec3(0.0f, 0.0f, 0.0f),   // gleda u centar
            glm::vec3(0.0f, 0.0f, -1.0f)    // up os
        );
        shader.SetUniform4x4("view", view);

        // ----------------------------------------------------
        // PROJECTION MATRIX – P ili O tipka
        // ----------------------------------------------------
        GLFWwindow* win = window.getWindow();
        if (glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS) usePerspective = true;
        if (glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS) usePerspective = false;

        glm::mat4 projection;

        if (usePerspective)
        {
            projection = glm::perspective(
                glm::radians(45.0f),
                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                0.1f, 100.0f
            );
        }
        else
        {
            projection = glm::ortho(
                -4.0f, 4.0f,
                -3.0f, 3.0f,
                0.1f, 100.0f
            );
        }

        shader.SetUniform4x4("projection", projection);

        // ----------------------------------------------------
        // RENDER Ukoliko želimo više instanci istog modela
        // ----------------------------------------------------
        for (int i = 0; i < positions.size(); i++)
        {
            glm::mat4 modelMat = glm::mat4(1.0f);

            // translacija
            modelMat = glm::translate(modelMat, positions[i]);

            // rotacija
            float angle = glfwGetTime() * (i + 1);
            modelMat = glm::rotate(modelMat, angle, glm::vec3(0, 1, 0));

            shader.SetUniform4x4("model", modelMat);

            model.Draw(shader, tex);
        }

        window.SwapAndPoll();
    }

    window.CloseWindow();

    return 0;
}