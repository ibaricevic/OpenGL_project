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


//glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
//glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

 glm::mat4 projection = glm::ortho(-3.0f, 3.0f, -4.0f, 4.0f, 0.1f, 100.0f);
 glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

std::vector<glm::vec3> positions = {
       { 0.0f, 0.0f, 0.0f},
       {-1.5f, 0.8f, 0.3f},
       { 1.0f, -1.2f, -1.3f},
       //{ 1.0f, 1.2f, 1.3f},
       //{ 0.0f, 0.0f, -10.0f},
       //{ -1.3f,  1.0f, -1.5f}
};

std::vector<glm::vec3> rotationAxes = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    //{-1.0f, 0.0f, 0.0f},
    //{0.0f, -1.0f, 0.0f},
    //{0.0f, 0.0f, -1.0f}
};


int main()
{
    Window window("Vjezba4", SCR_WIDTH, SCR_HEIGHT);

    Model model1("res/models/pyramid.obj");
    Model model2("res/models/kocka.obj");
    Model model3("res/models/dragon.obj");
    Shader shader("res/shaders/vShader.glsl", "res/shaders/fShader.glsl");
    Texture tex("res/textures/container.jpg");

    Renderer render;

    float offsetX = 0.0f;
    float offsetY = 0.0f;

    glm::vec3 color(1.0f, 1.0f, 1.0f);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!window.isClosed())
    {
        window.ProcessInput();
        render.Clear();

        shader.Bind();
        shader.SetUniformVec3("offset", offsetX, offsetY, 0.0f);
        shader.SetUniformVec3("color", color);

        

        std::vector<Model*> models = { &model1, &model2, &model3 };

        for (unsigned int i = 0; i < positions.size(); i++)
        {
            glm::mat4 mat_model = glm::mat4(1.0f);
            mat_model = glm::translate(mat_model, positions[i]);
            float angle = 10.0f * i;
            //float angle = glfwGetTime() * (i+5);
            mat_model = glm::rotate(mat_model, glm::radians(angle), rotationAxes[i]);

            const float radius = 5.0f;
            float camX = sin(glfwGetTime()) * radius;
            float camZ = cos(glfwGetTime()) * radius;
            glm::mat4 view;
            view = glm::lookAt(glm::vec3(camX, 0.0f, camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            shader.SetUniform4x4("view", view);
            shader.SetUniform4x4("projection", projection);
            shader.SetUniform4x4("model", mat_model);        

            models[i]->Draw(shader, tex);
        }

        window.SwapAndPoll();
    }

    window.CloseWindow();

    return 0;
}