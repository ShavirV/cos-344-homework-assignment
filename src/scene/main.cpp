// main.cpp  —  COS344 Miniature Golf Course
//
// Holes are flat red cuboids 28 (length) × 0.2 (height) × 15 (width).
// Rotation: 0° = runs along X,  90° = runs along Z.
//
// Directed play order:
//   1→2→3→4→5→6→7→8→9(tunnel)→10→11→12→13→14→15→16→17→18
//
//  Hole  | cx    | cz    | rot
//  ------+-------+-------+----
//  H1    | -80   | +40   |  0°   START
//  H2    | -80   | +15   |  0°
//  H3    | -48   | +40   |  0°
//  H4    | -48   | +15   |  0°
//  H5    | -80   |  -8   |  0°
//  H6    | -20   | +28   | 90°
//  H7    | -22   | -35   |  0°
//  H8    | -19   |  -8   | 90°
//  H9W   | -48   |  -8   |  0°   tunnel west approach
//  H9E   | +48   |  -8   |  0°   tunnel east exit
//  H10   | +22   | -35   |  0°
//  H11   | +55   | -35   |  0°
//  H12   | +80   |  -8   |  0°
//  H13   | +80   | +15   |  0°
//  H14   | +80   | +40   |  0°
//  H15   | +48   | +40   |  0°
//  H16   | +48   | +15   |  0°
//  H17   | +16   | +40   |  0°
//  H18   | +16   | +15   |  0°   FINISH
//
//  Volcano centre: (0, 20, -10)   base radius 20, height 40
//  Tunnel mouths:  x = ±20,  y = 4, z = -8  (cylinders rotated 90° around Z)

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

#include "../objects/Hole1.h"
#include "../objects/Hole2.h"
#include "../objects/Hole3.h"
#include "../objects/Hole4.h"
#include "../objects/Hole5.h"
#include "../objects/Hole6.h"
#include "../objects/Hole7.h"
#include "../objects/Hole8.h"
#include "../objects/Hole9E.h"
#include "../objects/Hole9W.h"
#include "../objects/Hole10.h"
#include "../objects/Hole11.h"
#include "../objects/Hole12.h"
#include "../objects/Hole13.h"
#include "../objects/Hole14.h"
#include "../objects/Hole15.h"
#include "../objects/Hole16.h"
#include "../objects/Hole17.h"
#include "../objects/Hole18.h"


const int WIDTH  = 1000;
const int HEIGHT = 1000;

// ── Camera ──────────────────────────────────────────────────────────────────
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
    if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) cameraPos += SPEED * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= SPEED * cameraUp;
}

int main()
{
    GLFWwindow* window;
    try { window = setUp(); }
    catch (const char* e) { std::cout << e << std::endl; throw; }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    GLuint shader = LoadShaders("src/shaders/vertex.glsl", "src/shaders/fragment.glsl");

    GLint locLightColor = glGetUniformLocation(shader, "uLightColor");
    GLint locAmbient    = glGetUniformLocation(shader, "uAmbient");

    // ── Ground ───────────────────────────────────────────────────────────────
    Mesh ground = Mesh::makeCuboid(400.0f, 0.5f, 150.0f, 0.35f, 0.20f, 0.08f, 1.0f);
    ground.setup();

    // ── Volcano ──────────────────────────────────────────────────────────────
    Mesh volcano = Mesh::makeCone(20.0f, 40.0f, 64, 0.40f, 0.26f, 0.10f, 1.0f);
    volcano.translate(0.0f, 20.0f, -10.0f);
    volcano.setup();

    // ── Holes — using Mesh::makeHole(cx, cy, cz, rotY) ──────────────────────
    // cy = 0.35 places the fairway flush on top of the ground slab (0.5 tall).
    const float FY = 0.35f;

    Hole1 h1;
    h1.translate(-180.0f, 0.35f, 60.0f); //cant rotate 

    Hole2 h2;
    h2.translate(-170.0f, 0.35f, 40.0f);

    Hole3 h3; 
    h3.translate(-180.0f, 0.35, 20.0f);

    Hole4 h4;
    h4.translate(-150.0f, 0.35f, 20.0f);

    Hole5 h5;
    h5.translate(-175.0f, 0.35, -16.0);

    Hole6 h6;
    h6.translate(-175.0f, 0.35f, -50.0f);

    Hole7 h7;
    h7.translate(-150.0f, 0.35f, -35.0f);

    Hole8 h8 ;
    h8.translate(-110.0f, 0.35f, -10.0f);

    Hole9E h9e;
    h9e.translate(-55.0f, 0.35, -10.0f);

    Hole9W h9w;
    h9w.translate(40.0f, 0.35, -10.0f);

    
    Hole10 h10; 
    h10.translate(80.0f, 0.35f, -10.0f);

    Hole11 h11;
    h11.translate(110.0f, 0.35f, 20.0f);

    Hole12 h12;
    h12.translate(150.0f, 0.35f, 25.0f);

    
    Hole13 h13;
    h13.init(shader);

    Hole14 h14;
    h14.init(shader);

    Hole15 h15;
    h15.init(shader);

    Hole16 h16;
    h16.translate(80.0f, 0.35f, 50.0f);

    Hole17 h17;
    h17.translate(120.0f, 0.35f, 50.0f);


    Hole18 h18;
    h18.translate(35.0f, 0.35f, 50.0f);
    

    // ── Shader uniforms ──────────────────────────────────────────────────────
    glUseProgram(shader);
    GLint locModel = glGetUniformLocation(shader, "model");
    GLint locView  = glGetUniformLocation(shader, "view");
    GLint locProj  = glGetUniformLocation(shader, "projection");
    GLint locCol   = glGetUniformLocation(shader, "colour");
    std::cout << "model=" << locModel << " view=" << locView
              << " projection=" << locProj << " colour=" << locCol << std::endl;

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 500.0f);

    // ── Render loop ──────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(locLightColor, 1.0f, 0.95f, 0.85f);   // warm sunlight
        glUniform3f(locAmbient,    0.15f, 0.15f, 0.20f);   // soft sky bounce

        ground.draw(shader, false);

        h1.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                      // camera position

        h2.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h3.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h4.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h5.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h6.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h7.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h8.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h9e.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h9w.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position


        h10.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h11.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h12.draw(shader, false,
        glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // light direction
        glm::vec3(1.0f, 0.95f, 0.85f),                 // light colour
        cameraPos);                                     //camera position

        h13.draw(view, projection,
         glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // sunDir
         cameraPos); 
         
         h14.draw(view, projection,
         glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // sunDir
         cameraPos); 
         
        h15.draw(view, projection,
         glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f)),  // sunDir
         cameraPos);   


         static double lastTime = glfwGetTime();
        double now      = glfwGetTime();
        float deltaTime = (float)(now - lastTime);
        lastTime = now;

        h16.update(deltaTime);
        h16.setCameraData(view, projection, cameraPos);
        h16.draw(shader, false);

        h17.draw(shader, false);


        h18.update(deltaTime);
        h18.setCameraData(view, projection, cameraPos);
        h18.draw(shader, false);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    glfwTerminate();
    return 0;
}