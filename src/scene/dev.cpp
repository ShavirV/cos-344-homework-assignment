// =============================================================================
// dev.cpp
// Standalone viewer for COS344 Practical Holes 13-15.
//
// Shows ONE hole at a time, framed to fill the viewport with a fly-camera
// for free exploration. There is no surrounding course, no other holes -
// just the selected hole displayed in isolation, which is the point of this
// test harness.
//
// Controls:
//   1 / 2 / 3   - switch to Hole 13 / 14 / 15
//   R           - re-frame camera on current hole
//   W / S       - fly forward / back
//   A / D       - strafe left / right
//   SPACE / LSHIFT - ascend / descend
//   Mouse       - look around (cursor is captured)
//   M           - release / re-capture the cursor
//   TAB         - toggle wireframe
//   ESC         - quit
//
// Only spec-allowed libraries are used. Shaders are written inline (the
// spec allows "additional code to the shaders to improve the efficiency of
// your rendering" and shader compilation is a normal OpenGL workflow).
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../objects/Hole13.h"
#include "../objects/Hole14.h"
#include "../objects/Hole15.h"

// -----------------------------------------------------------------------------
// Inline Blinn-Phong shader. The hole classes expect these exact uniform
// names: uMVP, uModel, uNormalMat, uLightDir, uLightColor, uAmbient, uViewPos.
// -----------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Upgraded Blinn-Phong shaders with emissive, lava point-light, rim light,
// procedural noise variation, tone-mapping, and gamma correction.
// ---------------------------------------------------------------------------
static const char* VERTEX_SRC =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aNormal;\n"
    "layout(location = 2) in vec3 aColor;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat4 uModel;\n"
    "uniform mat3 uNormalMat;\n"
    "out vec3 vWorldPos;\n"
    "out vec3 vNormal;\n"
    "out vec3 vColor;\n"
    "void main() {\n"
    "    vec4 wp = uModel * vec4(aPos, 1.0);\n"
    "    vWorldPos = wp.xyz;\n"
    "    vNormal = normalize(uNormalMat * aNormal);\n"
    "    vColor = aColor;\n"
    "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "}\n";

static const char* FRAGMENT_SRC =
    "#version 330 core\n"
    "in vec3 vWorldPos;\n"
    "in vec3 vNormal;\n"
    "in vec3 vColor;\n"
    "uniform vec3  uLightDir;\n"
    "uniform vec3  uLightColor;\n"
    "uniform vec3  uAmbient;\n"
    "uniform vec3  uViewPos;\n"
    "uniform float uEmissive;\n"
    "uniform vec3  uEmissiveColor;\n"
    "uniform vec3  uLavaLightPos;\n"
    "uniform vec3  uLavaLightColor;\n"
    "uniform float uLavaLightIntensity;\n"
    "out vec4 FragColor;\n"
    // Value-noise helpers
    "float hash3(vec3 p){\n"
    "    p=fract(p*vec3(127.1,311.7,74.7));\n"
    "    p+=dot(p,p.yxz+19.19);\n"
    "    return fract((p.x+p.y)*p.z);\n"
    "}\n"
    "float noise3(vec3 p){\n"
    "    vec3 i=floor(p), f=fract(p);\n"
    "    f=f*f*(3.0-2.0*f);\n"
    "    return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),\n"
    "                   mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),\n"
    "               mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),\n"
    "                   mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1,1,1)),f.x),f.y),f.z);\n"
    "}\n"
    "void main(){\n"
    "    vec3 N=normalize(vNormal);\n"
    "    vec3 L=normalize(uLightDir);\n"
    "    vec3 V=normalize(uViewPos-vWorldPos);\n"
    "    vec3 H=normalize(L+V);\n"
    // Micro-variation
    "    float nv=noise3(vWorldPos*2.5)*0.06-0.03;\n"
    "    vec3 base=clamp(vColor+nv,0.0,1.0);\n"
    // Diffuse + specular
    "    float diff=max(dot(N,L),0.0);\n"
    "    float spec=pow(max(dot(N,H),0.0),28.0)*0.18;\n"
    "    vec3 lit=base*(uAmbient+diff*uLightColor)+spec*uLightColor;\n"
    // Lava point light
    "    if(uLavaLightIntensity>0.001){\n"
    "        vec3 lv=uLavaLightPos-vWorldPos;\n"
    "        float ld=length(lv); lv/=ld;\n"
    "        float la=max(dot(N,lv),0.0);\n"
    "        float atten=uLavaLightIntensity/(1.0+0.04*ld*ld);\n"
    "        lit+=base*la*uLavaLightColor*atten;\n"
    "    }\n"
    // Rim light
    "    float rim=pow(1.0-max(dot(N,V),0.0),3.5)*0.12;\n"
    "    lit+=rim*uLightColor;\n"
    // Emissive override
    "    if(uEmissive>0.5){\n"
    "        float sh=noise3(vWorldPos*3.0)*0.3+0.7;\n"
    "        lit=base*uEmissiveColor*sh*3.5;\n"
    "    }\n"
    // Reinhard tone-map + gamma
    "    lit=lit/(lit+vec3(1.0));\n"
    "    lit=pow(clamp(lit,0.0,1.0),vec3(1.0/2.2));\n"
    "    FragColor=vec4(lit,1.0);\n"
    "}\n";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, 2048, NULL, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return s;
}

static GLuint buildProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   VERTEX_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, 2048, NULL, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// -----------------------------------------------------------------------------
// Fly camera
// -----------------------------------------------------------------------------
struct Camera {
    glm::vec3 pos;
    float yaw;    // radians; 0 looks down +Z
    float pitch;  // radians

    Camera() : pos(0.0f, 25.0f, -40.0f), yaw(0.0f), pitch(-0.45f) {}

    glm::vec3 forward() const {
        return glm::vec3(std::sin(yaw) * std::cos(pitch),
                         std::sin(pitch),
                         std::cos(yaw) * std::cos(pitch));
    }
    glm::vec3 right() const {
        return glm::normalize(glm::cross(forward(), glm::vec3(0, 1, 0)));
    }
    glm::mat4 view() const {
        return glm::lookAt(pos, pos + forward(), glm::vec3(0, 1, 0));
    }

    // Position the camera so it looks at `target` from a sensible distance
    // (slightly behind the tee, elevated, looking down the fairway).
    void frameOn(const glm::vec3& target, float radius) {
        pos = glm::vec3(target.x, target.y + radius * 0.55f,
                        target.z - radius * 0.95f);
        // Aim at target
        glm::vec3 dir = glm::normalize(target - pos);
        pitch = std::asin(dir.y);
        yaw   = std::atan2(dir.x, dir.z);
    }
};

static Camera g_cam;
static int    g_activeHole = 13;
static bool   g_wireframe  = false;
static double g_lastX = 0.0, g_lastY = 0.0;
static bool   g_firstMouse = true;
static bool   g_mouseLook  = true;
static int    g_winW = 1280;
static int    g_winH = 720;
static bool   g_needFrame = true;  // request a camera reframe on hole switch

// Forward declarations of the hole instances so callbacks can reference them.
static Hole13* g_h13 = NULL;
static Hole14* g_h14 = NULL;
static Hole15* g_h15 = NULL;

static void frameCurrent() {
    glm::vec3 c(0.0f); float r = 50.0f;
    if (g_activeHole == 13 && g_h13) { c = g_h13->sceneCenter(); r = g_h13->sceneRadius(); }
    if (g_activeHole == 14 && g_h14) { c = g_h14->sceneCenter(); r = g_h14->sceneRadius(); }
    if (g_activeHole == 15 && g_h15) { c = g_h15->sceneCenter(); r = g_h15->sceneRadius(); }
    g_cam.frameOn(c, r);
}

static void key_cb(GLFWwindow* w, int key, int /*sc*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, GLFW_TRUE);
    if (key == GLFW_KEY_1) { g_activeHole = 13; g_needFrame = true; }
    if (key == GLFW_KEY_2) { g_activeHole = 14; g_needFrame = true; }
    if (key == GLFW_KEY_3) { g_activeHole = 15; g_needFrame = true; }
    if (key == GLFW_KEY_R) g_needFrame = true;
    if (key == GLFW_KEY_TAB) {
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
    }
    if (key == GLFW_KEY_M) {
        g_mouseLook = !g_mouseLook;
        glfwSetInputMode(w, GLFW_CURSOR,
                         g_mouseLook ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        g_firstMouse = true;
    }
}

static void cursor_cb(GLFWwindow* /*w*/, double x, double y) {
    if (!g_mouseLook) return;
    if (g_firstMouse) { g_lastX = x; g_lastY = y; g_firstMouse = false; return; }
    double dx = x - g_lastX, dy = y - g_lastY;
    g_lastX = x; g_lastY = y;
    const float sens = 0.0025f;
    g_cam.yaw   += (float)dx * sens;
    g_cam.pitch -= (float)dy * sens;
    if (g_cam.pitch >  1.5f) g_cam.pitch =  1.5f;
    if (g_cam.pitch < -1.5f) g_cam.pitch = -1.5f;
}

static void size_cb(GLFWwindow* /*w*/, int width, int height) {
    g_winW = width; g_winH = height;
    glViewport(0, 0, width, height);
}

static void processMovement(GLFWwindow* w, float dt) {
    float speed = 18.0f;
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 3.5f;
    glm::vec3 f = g_cam.forward();
    glm::vec3 fFlat(f.x, 0.0f, f.z);
    if (glm::length(fFlat) > 1e-4f) fFlat = glm::normalize(fFlat);
    glm::vec3 r = g_cam.right();

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) g_cam.pos += fFlat * speed * dt;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) g_cam.pos -= fFlat * speed * dt;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) g_cam.pos -= r * speed * dt;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) g_cam.pos += r * speed * dt;
    if (glfwGetKey(w, GLFW_KEY_SPACE)      == GLFW_PRESS) g_cam.pos.y += speed * dt;
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) g_cam.pos.y -= speed * dt;
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for macOS

    GLFWwindow* win = glfwCreateWindow(g_winW, g_winH,
        "COS344 - Holes 13-15 (press 1/2/3, R to reframe)", NULL, NULL);
    if (!win) {
        std::cerr << "Window creation failed\n";
        glfwTerminate(); return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        glfwTerminate(); return 1;
    }
    glGetError(); // clear post-init GLEW error

    glfwSetKeyCallback(win, key_cb);
    glfwSetCursorPosCallback(win, cursor_cb);
    glfwSetFramebufferSizeCallback(win, size_cb);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_MULTISAMPLE);

    GLuint program = buildProgram();

    Hole13 hole13;  g_h13 = &hole13;
    Hole14 hole14;  g_h14 = &hole14;
    Hole15 hole15;  g_h15 = &hole15;
    hole13.init(program, "volcano.glb");  // path relative to working dir
    hole14.init(program);
    hole15.init(program);

    double prevTime = glfwGetTime();
    std::cout << "Controls: 1/2/3 switch hole; R reframe; WASD + Space/Shift fly; "
                 "mouse look; TAB wireframe; M release mouse; ESC quit.\n";

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - prevTime);
        prevTime = now;

        if (g_needFrame) { frameCurrent(); g_needFrame = false; }

        processMovement(win, dt);

        // Light sky-blue background (so the holes read as outdoor scenes)
        glClearColor(0.55f, 0.75f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                          (float)g_winW / (float)g_winH,
                                          0.1f, 500.0f);
        glm::mat4 view = g_cam.view();

        // Daytime sun direction (report 7.1: directional light at high angle)
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.85f, 0.4f));

        switch (g_activeHole) {
            case 13: hole13.draw(view, proj, sunDir, g_cam.pos); break;
            case 14: hole14.draw(view, proj, sunDir, g_cam.pos); break;
            case 15: hole15.draw(view, proj, sunDir, g_cam.pos); break;
        }

        std::ostringstream oss;
        oss << "COS344 - Hole " << g_activeHole
            << "  |  cam (" << std::fixed << std::setprecision(1)
            << g_cam.pos.x << ", " << g_cam.pos.y << ", " << g_cam.pos.z << ")";
        glfwSetWindowTitle(win, oss.str().c_str());

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    hole13.cleanup();
    hole14.cleanup();
    hole15.cleanup();
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}