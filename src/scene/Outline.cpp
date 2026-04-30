

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <chrono> 

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../../include/Triangle.h"
#include "../../include/Rectangle.h"
#include "../../include/Circle.h"
#include "../core/shader.hpp"




using namespace glm;
using namespace std;

const char *getError()
{
    const char *errorDescription;
    glfwGetError(&errorDescription);
    return errorDescription;
}

inline void startUpGLFW()
{
    if (!glfwInit())
    {
        throw getError();
    }
}

inline void startUpGLEW()
{
    std::cout << "startup glew" << std::endl;
    glewExperimental = true;
    GLenum err = glewInit();
    // if (err != GLEW_OK)
    // {
    //     std::cout << "glew failed to start" << std::endl;
    //     glfwTerminate();
    //     throw (const char*)glewGetErrorString(err);  // GLEW's own error
    // }
    glGetError();
}

inline GLFWwindow *setUp()
{
    std::cout<< "Starting setup" << std::endl;

    startUpGLFW();
    glfwWindowHint(GLFW_SAMPLES, 4);               // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
    GLFWwindow *window;                                            // (In the accompanying source code, this variable is global for simplicity)

    std::cout << "Made window variable" << std::endl;
    window = glfwCreateWindow(1000, 1000, "u23621372", NULL, NULL);
    if (window == NULL)
    {
        cout << getError() << endl;
        glfwTerminate();
        throw "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n";
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    std::cout << "GLEW initialized" << std::endl;
    startUpGLEW();
    std::cout << "complete setup" << std::endl;
    return window;
}

int main()
{
    GLFWwindow *window;
    try
    {
        window = setUp();
    }
    catch (const char *e)
    {
        cout << e << endl;
        throw;
    }

    glClearColor(0, 0, 0, 1);

    GLuint shaders = LoadShaders("src/shaders/vertex.glsl", "src/shaders/fragments.glsl");
    


    bool wireframe = false;
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glClear (GL_COLOR_BUFFER_BIT);//background color


        


        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
}