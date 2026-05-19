#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../include/shader.hpp"
#include "../../include/Mesh.h"

const int WIDTH  = 1000;
const int HEIGHT = 1000;

const char* getError()
{
    const char* errorDescription;
    glfwGetError(&errorDescription);
    return errorDescription;
}

inline void startUpGLFW()
{
    if (!glfwInit())
        throw getError();
}

inline void startUpGLEW()
{
    glewExperimental = true;
    glewInit();
    glGetError();
}

inline GLFWwindow* setUp()
{
    startUpGLFW();

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Miniature Golf Course", NULL, NULL);
    if (window == NULL)
    {
        std::cout << getError() << std::endl;
        glfwTerminate();
        throw "Failed to open GLFW window.";
    }

    glfwMakeContextCurrent(window);
    startUpGLEW();

    return window;
}

int main()
{
    GLFWwindow* window;
    try
    {
        window = setUp();
    }
    catch (const char* e)
    {
        std::cout << e << std::endl;
        throw;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    GLuint shader = LoadShaders("src/shaders/vertex.glsl", "src/shaders/fragment.glsl");

    // ── Ground plane ─────────────────────────────────────────────────────────
    Mesh ground = Mesh::makeCuboid(20.0f, 0.1f, 20.0f,
                                   0.13f, 0.55f, 0.13f, 1.0f);
    ground.setup();

    // Debug: confirm vertices were added
    std::cout << "Ground vertex count: " << ground.getVertexCount() << std::endl;
    std::cout << "Ground index count:  " << ground.getIndexCount()  << std::endl;

    // Debug: confirm uniforms exist in shader
    glUseProgram(shader);
    GLint locModel = glGetUniformLocation(shader, "model");
    GLint locView  = glGetUniformLocation(shader, "view");
    GLint locProj  = glGetUniformLocation(shader, "projection");
    GLint locCol   = glGetUniformLocation(shader, "colour");
    std::cout << "model="      << locModel
              << " view="      << locView
              << " projection="<< locProj
              << " colour="    << locCol << std::endl;

    // ── Camera ───────────────────────────────────────────────────────────────
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WIDTH / (float)HEIGHT,
        0.1f, 100.0f
    );

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 15.0f, 15.0f),
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(0.0f,  1.0f,  0.0f)
    );

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        ground.draw(shader, false);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    glfwTerminate();
    return 0;
}