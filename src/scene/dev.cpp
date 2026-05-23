// =============================================================================
// dev.cpp
// Standalone viewer for COS344 Practical Hole 13.
//
// Shows the current hole(s) framed in the viewport with a fly-camera for
// free exploration. Right now only Hole 13 is wired in; Hole 14 / 15 will
// be added the same way once they exist. Look for the "ADD HOLE 14/15"
// markers below — adding a new hole is a five-line change in each spot.
//
// Controls:
//   1           - switch to Hole 13   (2 / 3 will switch to 14 / 15 later)
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
// ADD HOLE 14/15: when those classes exist, add their #includes here.
// #include "../objects/Hole14.h"
// #include "../objects/Hole15.h"


// -----------------------------------------------------------------------------
// Inline Blinn-Phong shader. The hole classes expect these exact uniform
// names: uMVP, uModel, uNormalMat, uLightDir, uLightColor, uAmbient, uViewPos.
// -----------------------------------------------------------------------------
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

// Fragment shader.
//
// Adds procedural-noise "textures" for the turf and kerb materials, and a
// hemisphere ambient term so shadows pick up a hint of sky-blue while lit
// surfaces tilt warm. The shader identifies the material from the base
// vertex colour (no extra attribute or uniform required), so Hole13's
// geometry code does not need to change to opt in.
//
// All noise is computed in code via integer-coord hashing — no texture
// uploads, in line with the spec's no-prebuilt-textures rule.
static const char* FRAGMENT_SRC = R"GLSL(
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightDir;     // direction TO the sun
uniform vec3 uLightColor;   // sun colour
uniform vec3 uAmbient;      // base ambient term (used as the "ground bounce" colour)
uniform vec3 uViewPos;      // camera position in world space

out vec4 FragColor;

// ── Hash + value noise ──────────────────────────────────────────────────────
// Cheap deterministic 2D hash. Not cryptographic — just stable per-coordinate
// pseudo-random values to drive the noise.
float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// Single-octave value noise: sample the four integer-corner hash values and
// smooth-interpolate between them.
float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    // Smoothstep weights (Hermite) to soften the bilinear blend.
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Fractal Brownian Motion — three octaves of value noise. Gives the
// multi-scale look real materials have: small detail + bigger blotches.
float fbm(vec2 p) {
    float v = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 3; ++i) {
        v += amp * vnoise(p);
        p *= 2.07;        // non-integer ratio avoids alignment artefacts
        amp *= 0.5;
    }
    return v;
}

// ── Material-specific shading ───────────────────────────────────────────────

// Turf: per-blade colour variation + larger wear patches.
// World-space XZ is used so the noise "sticks" to the ground as the camera
// moves (it would crawl with view-space coords).
vec3 shadeTurf(vec3 baseGreen, vec3 worldPos) {
    vec2 uv = worldPos.xz;

    // High-frequency variation — individual blade-clump tints.
    float n1 = vnoise(uv * 8.0);
    // Medium variation — patchy wear, slightly drier areas.
    float n2 = fbm(uv * 0.6);

    // Tint between a slightly bluer-green and a slightly yellower-green.
    vec3 coolGreen = baseGreen * vec3(0.85, 1.00, 0.70);  // healthy / shaded
    vec3 warmGreen = baseGreen * vec3(1.15, 1.00, 0.55);  // sun-bleached / dry
    vec3 tinted    = mix(coolGreen, warmGreen, n1);

    // Darker wear patches around 30% of the surface.
    float wear = smoothstep(0.45, 0.65, n2);
    tinted *= mix(1.0, 0.70, wear);

    // A touch of fine "blade speckle" — small brightness jitter.
    float speckle = vnoise(uv * 35.0) * 0.10 - 0.05;
    tinted += vec3(speckle * 0.5, speckle, speckle * 0.3);

    return tinted;
}

// Concrete kerb: aggregate speckle (fine) + weathering blotches (coarse).
vec3 shadeKerb(vec3 baseGrey, vec3 worldPos) {
    vec2 uv = worldPos.xz;

    // Fine aggregate — high-frequency noise, ±10% brightness jitter.
    float aggregate = vnoise(uv * 25.0) * 0.20 - 0.10;
    // Larger weathering — slow stains and water marks.
    float weather   = fbm(uv * 0.8);
    // Slightly cool weathered patches, slightly warm fresh patches.
    vec3 fresh   = baseGrey * 1.05;
    vec3 stained = baseGrey * vec3(0.85, 0.88, 0.92);  // cooler grey
    vec3 tinted  = mix(stained, fresh, smoothstep(0.35, 0.65, weather));

    tinted += vec3(aggregate);
    return tinted;
}

// ── Material identification from the base colour ────────────────────────────
// Green-dominant high-saturation → turf. Roughly grey (R≈G≈B and bright) → kerb.
// Anything else falls through to "use the colour as-is" — covers the tee
// patch, cup, flag, pole.
bool looksLikeTurf(vec3 c) {
    return c.g > c.r + 0.15 && c.g > c.b + 0.15 && c.g > 0.30;
}
bool looksLikeKerb(vec3 c) {
    float maxc = max(c.r, max(c.g, c.b));
    float minc = min(c.r, min(c.g, c.b));
    return (maxc - minc) < 0.10 && maxc > 0.50;
}

void main() {
    // Material-specific base colour with procedural variation.
    vec3 base;
    if (looksLikeTurf(vColor))      base = shadeTurf(vColor, vWorldPos);
    else if (looksLikeKerb(vColor)) base = shadeKerb(vColor, vWorldPos);
    else                            base = vColor;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // ── Hemisphere ambient ───────────────────────────────────────────────────
    // Surfaces facing up pick up sky-blue; surfaces facing down pick up the
    // ground-bounce colour passed in via uAmbient. Reads as outdoor light
    // without needing a full GI solution.
    vec3 skyColor    = vec3(0.55, 0.70, 0.95);            // hardcoded noon sky
    vec3 groundBounce = uAmbient * 1.20;                  // tint from uAmbient
    float hemi = 0.5 + 0.5 * N.y;                          // 1 facing up, 0 down
    vec3 ambient = mix(groundBounce, skyColor, hemi) * 0.55;

    // ── Diffuse (Lambert) ────────────────────────────────────────────────────
    float diff = max(dot(N, L), 0.0);

    // ── Specular (Blinn-Phong) ───────────────────────────────────────────────
    // Turf is matte → very low spec power, almost no contribution.
    // Kerb is slightly glossy concrete → moderate.
    // Other materials (cup, flag) → default.
    float specStrength = 0.10;
    float specPower    = 24.0;
    if (looksLikeTurf(vColor)) { specStrength = 0.02; specPower = 8.0;  }
    if (looksLikeKerb(vColor)) { specStrength = 0.18; specPower = 32.0; }
    float spec = pow(max(dot(N, H), 0.0), specPower) * specStrength;

    // Combine, then gently roll off highlights so the noon sun on bright
    // turf doesn't blow to pure white. Identity for shadowed pixels,
    // compressive only at the top.
    vec3 lit = base * (ambient + diff * uLightColor) + spec * uLightColor;
    lit = lit / (1.0 + 0.4 * lit);

    FragColor = vec4(lit, 1.0);
}
)GLSL";

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
// ADD HOLE 14/15: declare g_h14 / g_h15 pointers here when those classes exist.

static void frameCurrent() {
    glm::vec3 c(0.0f); float r = 50.0f;
    if (g_activeHole == 13 && g_h13) { c = g_h13->sceneCenter(); r = g_h13->sceneRadius(); }
    // ADD HOLE 14/15: add the matching `if` lines here.
    g_cam.frameOn(c, r);
}

static void key_cb(GLFWwindow* w, int key, int /*sc*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, GLFW_TRUE);
    if (key == GLFW_KEY_1) { g_activeHole = 13; g_needFrame = true; }
    // ADD HOLE 14/15: bind keys 2 / 3 here.
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
        "COS344 - Hole 13 (press 1 to select, R to reframe)", NULL, NULL);
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
    hole13.init(program);
    // ADD HOLE 14/15: declare, point g_h14/g_h15 at, and init the new instances here.

    double prevTime = glfwGetTime();
    std::cout << "Controls: 1 selects Hole 13; R reframe; WASD + Space/Shift fly; "
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

        // Noon sun direction (near-vertical, slight tilt so kerb sides still get hit).
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.15f, 0.96f, 0.25f));

        switch (g_activeHole) {
            case 13: hole13.draw(view, proj, sunDir, g_cam.pos); break;
            // ADD HOLE 14/15: add the matching case branches here.
            default: hole13.draw(view, proj, sunDir, g_cam.pos); break;
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
    // ADD HOLE 14/15: clean up the new instances here.
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}