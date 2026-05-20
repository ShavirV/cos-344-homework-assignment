#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../core/shader.hpp"
#include "../core/Mesh.h"

const int WIDTH  = 1000;
const int HEIGHT = 1000;

// ── Camera state ─────────────────────────────────────────────────────────────
glm::vec3 cameraPos   = glm::vec3(0.0f, 20.0f, 50.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, -20.0f, -50.0f));
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
const float SPEED     = 0.5f;
const float LOOK_SPEED = 1.0f;   // degrees per frame

float yaw   = -90.0f;   // start facing -Z
float pitch = -21.8f;   // start angled slightly down (matches original front vector)

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

void processInput(GLFWwindow* window)
{
    // ── Look (arrow keys) ────────────────────────────────────────────────────
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) yaw   -= LOOK_SPEED;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw   += LOOK_SPEED;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) pitch += LOOK_SPEED;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) pitch -= LOOK_SPEED;

    // Clamp pitch so the camera doesn't flip over
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Rebuild cameraFront from yaw/pitch
    cameraFront = glm::normalize(glm::vec3(
        cosf(glm::radians(yaw)) * cosf(glm::radians(pitch)),
        sinf(glm::radians(pitch)),
        sinf(glm::radians(yaw)) * cosf(glm::radians(pitch))
    ));

    // ── Move (WASD) ──────────────────────────────────────────────────────────
    glm::vec3 flat  = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flat, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += SPEED * flat;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= SPEED * flat;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= SPEED * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += SPEED * right;
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
    Mesh ground = Mesh::makeCuboid(160.0f, 0.1f, 100.0f,
                                   0.13f, 0.55f, 0.13f, 1.0f);
    ground.setup();

    Mesh hole1 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole1.rotate(90, 0.0f, 1.0f, 0.0f);
    hole1.translate(-30.0f, 0.1f, -70.0f);
    hole1.setup();

    Mesh hole2 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole2.rotate(45, 0.0f, 1.0f, 0.0f);
    hole2.translate(-40.0f, 0.1f, -50.0f);
    hole2.setup();

 
    Mesh hole3 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
     hole3.translate(-60.0f, 0.1f, -35.0f);
    hole3.setup();

    Mesh hole4 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole4.rotate(90, 0.0f, 1.0f, 0.0f);
    hole4.translate(13.0f, 2.0f, -39.0f);
    hole4.setup();

    Mesh hole5 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole5.rotate(-20, 0.0f, 1.0f, 0.0f);
    hole5.translate(-15.0f, 5.0f, 27.0f);
    hole5.setup();

    Mesh hole6 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole6.rotate(20, 0.0f, 1.0f, 0.0f);
    hole6.translate(5.0f, 7.0f, 23.0f);

    Mesh hole7 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole7.rotate(90, 0.0f, 1.0f, 0.0f);
    hole7.translate(10.0f, 9.0f, 20.0f);

    Mesh hole8 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole8.translate(30.0f, 10.0f, -40.0f);

    Mesh hole9 = Mesh::makeCuboid(30.0f, 0.1f, 15.0f,
                                  1.0f, 0.0f, 0.0f, 1.0f);
    hole9.rotate(-30, 0.0f, 1.0f, 0.0f);
    hole9.translate(40.0f, 10.0f, -60.0f);

    Mesh volcano = Mesh::makeCone(30.0f, 30.0f, 64,
                                  0.39f, 0.26f, 0.13f, 0.5f);
    volcano.translate(-5.0f, 10.0f, -15.0f);
    volcano.setup();

    glUseProgram(shader);
    GLint locModel = glGetUniformLocation(shader, "model");
    GLint locView  = glGetUniformLocation(shader, "view");
    GLint locProj  = glGetUniformLocation(shader, "projection");
    GLint locCol   = glGetUniformLocation(shader, "colour");
    std::cout << "model=" << locModel << " view=" << locView
              << " projection=" << locProj << " colour=" << locCol << std::endl;

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WIDTH / (float)HEIGHT,
        0.1f, 200.0f
    );

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        ground.draw(shader, false);
        hole1.draw(shader, false);
        hole2.draw(shader, false);
        hole3.draw(shader, false);
        hole4.draw(shader, false);
        hole5.draw(shader, false);
        hole6.draw(shader, false);
        hole7.draw(shader, false);
        hole8.draw(shader, false);
        hole9.draw(shader, false);
        volcano.draw(shader, false);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    glfwTerminate();
    return 0;
}