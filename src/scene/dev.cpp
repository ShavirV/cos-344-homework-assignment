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

#include "../objects/Hole9E.h"
#include "../objects/Hole9W.h"
#include "../objects/Hole16.h"
#include "../objects/Hole17.h"
#include "../objects/Hole18.h"
#include "../objects/Water.h"
#include "../objects/Terrain.h"
#include "../objects/TerrainExample.hpp"
#include "../objects/Skybox.h"


const int WIDTH  = 1280;
const int HEIGHT = 720;
bool wireframeMode = false;

// ── Camera ────────────────────────────────────────────────────────────────────
glm::vec3 cameraPos   = glm::vec3(0.0f, 90.0f, 160.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, -90.0f, -160.0f));
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
const float SPEED      = 0.5f;
const float LOOK_SPEED = 1.0f;
float yaw   = -90.0f;
float pitch = -29.0f;

const char* getError() { const char* d; glfwGetError(&d); return d; }
inline void startUpGLFW() { if (!glfwInit()) throw getError(); }
inline void startUpGLEW() { glewExperimental = true; glewInit(); glGetError(); }

inline GLFWwindow* setUp()
{
    startUpGLFW();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(WIDTH, HEIGHT, "Miniature Golf Course", NULL, NULL);
    if (!w) { std::cout << getError() << std::endl; glfwTerminate(); throw "Failed to open window."; }
    glfwMakeContextCurrent(w);
    startUpGLEW();
    return w;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) yaw   -= LOOK_SPEED;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw   += LOOK_SPEED;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) pitch += LOOK_SPEED;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) pitch -= LOOK_SPEED;
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    cameraFront = glm::normalize(glm::vec3(
        cosf(glm::radians(yaw))  * cosf(glm::radians(pitch)),
        sinf(glm::radians(pitch)),
        sinf(glm::radians(yaw))  * cosf(glm::radians(pitch))));

    glm::vec3 flat  = glm::normalize(glm::vec3(cameraFront.x, 0.f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flat, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += SPEED * flat;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= SPEED * flat;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= SPEED * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += SPEED * right;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos.y += SPEED;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos.y -= SPEED;
    if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) cameraPos += SPEED * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= SPEED * cameraUp;

    static bool enterPressed = false;
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        if (!enterPressed) {
            wireframeMode = !wireframeMode;
            if (wireframeMode) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                std::cout << "Wireframe mode ON" << std::endl;
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                std::cout << "Wireframe mode OFF" << std::endl;
            }
            enterPressed = true;
        }
    } else {
        enterPressed = false;
    }
}

int main()
{
    GLFWwindow* window;
    try { window = setUp(); }
    catch (const char* e) { std::cout << e << std::endl; throw; }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // ── Fix 1: dark grey background instead of sky blue ──────────────────────
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    GLuint shader = LoadShaders("src/shaders/vertex.glsl", "src/shaders/fragment.glsl");

    // ── Fix 2: cache uniform locations BEFORE the render loop ────────────────
    glUseProgram(shader);
    GLint locView  = glGetUniformLocation(shader, "view");
    GLint locProj  = glGetUniformLocation(shader, "projection");

    // ── Projection matrix (constant, set once) ────────────────────────────────
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 500.0f);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));

    Mesh ground = Mesh::makeCuboid(2100.0f, 0.5f, 1250.0f, 0.13f, 0.55f, 0.13f, 1.0f);
    ground.setup();
        
       //Hole16 hole16;
       //Hole17 hole17;
       //Hole18 hole18;
       TerrainExample terrainExample;
       terrainExample.init();

       Mesh hole16platform = Mesh::makeHole();
       float lastTime = (float)glfwGetTime();

        Skybox skybox;
        skybox.init(
            "src/shaders/skybox.vert",
            "src/shaders/skybox.frag",
            "src/assets/textures/sky_day.bmp",
            "src/assets/textures/sky_night.bmp"
        );
        skybox.dayLengthSecs = 60.0f;  // 2-minute full day cycle — adjust to taste

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentTime = (float)glfwGetTime();
        float deltaTime   = currentTime - lastTime;
        lastTime          = currentTime;

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUseProgram(shader);
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));


        skybox.update(deltaTime);
        glDepthFunc(GL_LEQUAL);   // ← sky depth trick requires this
        skybox.draw(view, projection);
        glDepthFunc(GL_LESS);   // uses view/projection already computed above

        SkyLight sun  = skybox.getSun();
        SkyLight moon = skybox.getMoon();

        // Active light = whichever is above the horizon
        float sunWeight  = sun.intensity;
        float moonWeight = moon.intensity;

        glm::vec3 lightPos   = sunWeight > moonWeight ? sun.position  : moon.position;
        glm::vec3 lightColor = sunWeight > moonWeight ? sun.color     : moon.color;
        float     lightInt   = sunWeight > moonWeight ? sunWeight      : moonWeight;

        // Upload to your vertex/fragment shader (check your uniform names in
        // vertex.glsl / fragment.glsl and match them here):
        GLint locLightPos   = glGetUniformLocation(shader, "lightPos");
        GLint locLightColor = glGetUniformLocation(shader, "lightColor");
        GLint locLightInt   = glGetUniformLocation(shader, "lightIntensity");
        if (locLightPos   >= 0) glUniform3fv(locLightPos,   1, glm::value_ptr(lightPos));
        if (locLightColor >= 0) glUniform3fv(locLightColor, 1, glm::value_ptr(lightColor));
        if (locLightInt   >= 0) glUniform1f (locLightInt,   lightInt);

        //for the water redraw
        // hole16.setCameraData(view, projection, cameraPos);
        // hole16.update(deltaTime);

        // hole18.setCameraData(view, projection, cameraPos);
        // hole18.update(deltaTime);

        //hole17.draw(shader, false);
        terrainExample.draw(shader, false);
        

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    glfwTerminate();
    return 0;
}