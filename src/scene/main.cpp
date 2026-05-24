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

    // ── Ground ───────────────────────────────────────────────────────────────
    Mesh ground = Mesh::makeCuboid(210.0f, 0.5f, 125.0f, 0.13f, 0.55f, 0.13f, 1.0f);
    ground.setup();

    // ── Volcano ──────────────────────────────────────────────────────────────
    Mesh volcano = Mesh::makeCone(20.0f, 40.0f, 64, 0.40f, 0.26f, 0.10f, 1.0f);
    volcano.translate(0.0f, 20.0f, -10.0f);
    volcano.setup();

    // ── Tunnel mouths (hole 9 passes through volcano base along X at z=-8) ──
    Mesh tunnelW = Mesh::makeCylinder(3.5f, 5.5f, 32, 0.22f, 0.14f, 0.05f, 1.0f);
    tunnelW.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    tunnelW.translate(-20.0f, 4.0f, -8.0f);
    tunnelW.setup();

    Mesh tunnelE = Mesh::makeCylinder(3.5f, 5.5f, 32, 0.22f, 0.14f, 0.05f, 1.0f);
    tunnelE.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    tunnelE.translate( 20.0f, 4.0f, -8.0f);
    tunnelE.setup();

    // ── Holes — using Mesh::makeHole(cx, cy, cz, rotY) ──────────────────────
    // cy = 0.35 places the fairway flush on top of the ground slab (0.5 tall).
    const float FY = 0.35f;

    Mesh hole1 = Mesh::makeHole();
    hole1.rotate(0.0f, 0.0f, 1.0f, 0.0f);
    hole1.translate(-80.0f, FY, +40.0f);   // START

    Mesh hole2 = Mesh::makeHole();
    hole2.translate(-80.0f, FY, +15.0f);

    Mesh hole3 = Mesh::makeHole();
    hole3.translate(-48.0f, FY, +40.0f);

    Mesh hole4 = Mesh::makeHole();
    hole4.translate(-48.0f, FY, +15.0f);

    Mesh hole5 = Mesh::makeHole();
    hole5.translate(-80.0f, FY,  -8.0f);

    Mesh hole6 = Mesh::makeHole();
    hole6.rotate(90.0f, 0.0f, 1.0f, 0.0f);
    hole6.translate(-20.0f, FY, +28.0f);

    Mesh hole7 = Mesh::makeHole();
    hole7.translate(-22.0f, FY, -35.0f);

    Mesh hole8 = Mesh::makeHole();
    hole8.rotate(90.0f, 0.0f, 1.0f, 0.0f);
    hole8.translate(-19.0f, FY,  -8.0f);

    // Hole 9 — tunnel west and east use their own classes (with borders + rock)
    Mesh hole9w = Mesh::makeHole();
    hole9w.translate(-48.0f, FY, -8.0f);

    Mesh hole9e = Mesh::makeHole();
    hole9e.translate(+48.0f, FY, -8.0f);

    Mesh hole10 = Mesh::makeHole();
    hole10.translate(+22.0f, FY, -35.0f);

    Mesh hole11 = Mesh::makeHole();
    hole11.translate(+55.0f, FY, -35.0f);

    Mesh hole12 = Mesh::makeHole();
    hole12.translate(+80.0f, FY,  -8.0f);

    Mesh hole13 = Mesh::makeHole();
    hole13.translate(+80.0f, FY, +15.0f);

    Mesh hole14 = Mesh::makeHole();
    hole14.translate(+80.0f, FY, +40.0f);

    Mesh hole15 = Mesh::makeHole();
    hole15.translate(+48.0f, FY, +40.0f);

    Mesh hole16 = Mesh::makeHole();
    hole16.translate(+48.0f, FY, +15.0f);

    Mesh hole17 = Mesh::makeHole();
    hole17.translate(+16.0f, FY, +40.0f);

    Mesh hole18 = Mesh::makeHole();
    hole18.translate(+16.0f, FY, +15.0f);  // FINISH

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

        ground.draw(shader, false);

        volcano.draw(shader, false);
        tunnelW.draw(shader, false);
        tunnelE.draw(shader, false);

        hole1.draw(shader,  false);
        hole2.draw(shader,  false);
        hole3.draw(shader,  false);
        hole4.draw(shader,  false);
        hole5.draw(shader,  false);
        hole6.draw(shader,  false);
        hole7.draw(shader,  false);
        hole8.draw(shader,  false);
        hole9w.draw(shader, false);
        hole9e.draw(shader, false);
        hole10.draw(shader, false);
        hole11.draw(shader, false);
        hole12.draw(shader, false);
        hole13.draw(shader, false);
        hole14.draw(shader, false);
        hole15.draw(shader, false);
        hole16.draw(shader, false);
        hole17.draw(shader, false);
        hole18.draw(shader, false);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    glfwTerminate();
    return 0;
}