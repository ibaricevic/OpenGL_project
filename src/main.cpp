#include <iostream>
#include <string>

#include "Window.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/glm.hpp"

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 1000;

// 3D transformations
// glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -7.0f, 7.0f, 0.1f, 100.0f);
// glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));

glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

glm::vec3 cubePositions[] = {
    glm::vec3(-2.0f, 0.0f, 0.0f),
    glm::vec3(-1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(2.0f, 0.0f, 0.0f)
};

//polozaj kamere
const float radius = 6.0f;
glm::vec3 cameraTarget(cubePositions[2]);
//tangens 45 je 1 kada je y = z (skica)
glm::vec3 cameraPosition(0.0f, 2.0f, 5.0f);
glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0.0, 1.0, 0.0));

//boja svjetla
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

//boje objekata
glm::vec3 objectColor[] = {
    glm::vec3(0.5f, 0.5f, 0.5f),
    glm::vec3(1.0f, 0.5f, 0.31f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(1.0f, 0.0f, 0.0f)
};

//sjajnost objekata
float specularStrength[] = {
    0.5f,
    1.0f,
    0.3f,
    0.9f,
    0.3f
};


int main()
{
    Window window("Vjezba5", SCR_WIDTH, SCR_HEIGHT);

    glEnable(GL_DEPTH_TEST);

    Model model("res/models/kocka.obj");
    Model lightModel("res/models/kocka.obj");
    Shader shader("res/shaders/vShader.glsl", "res/shaders/fShader.glsl");
    Texture tex("res/textures/container.jpg");

    Renderer render;


    while (!window.isClosed())
    {
        window.ProcessInput();
        render.Clear();

        float camX = sin(glfwGetTime()) * radius;
        float camZ = cos(glfwGetTime()) * radius;
        // glm::vec3 cameraPosition(camX, cameraRoll, camZ);
        // glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0.0, 1.0, 0.0));

        glm::vec3 lightPos(camX, 5.0f, camZ);

        shader.Bind();
        shader.SetUniform4x4("projection", projection);
        shader.SetUniform4x4("view", view);


        glm::mat4 mat_lightModel = glm::mat4(1.0f);
        mat_lightModel = glm::translate(mat_lightModel, lightPos);
        mat_lightModel = glm::scale(mat_lightModel, glm::vec3(0.2f));

        shader.SetUniform4x4("model", mat_lightModel);
        shader.SetUniformVec3("lightColor", lightColor);
        shader.SetUniformVec3("lightPos", lightPos);
        shader.SetUniformVec3("viewPos", cameraPosition);

        lightModel.Draw(shader, tex);

        for (unsigned int i = 0; i < 4; i++)
        {
            glm::mat4 mat_model = glm::mat4(1.0f);
            mat_model = glm::translate(mat_model, cubePositions[i]);
            mat_model = glm::scale(mat_model, glm::vec3(0.5f));
            // float angle = 90.0f * i;
            shader.SetUniform4x4("model", mat_model);
            shader.SetUniformVec3("objectColor", objectColor[i]);
            shader.SetUniformFloat("specularStrength", specularStrength[i]);


            model.Draw(shader, tex);
        }


        window.SwapAndPoll();
    }

    window.CloseWindow();

    return 0;
}