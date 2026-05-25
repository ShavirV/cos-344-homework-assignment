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
#include "../objects/Volcano.hpp"
#include "../objects/Trex.hpp"
#include "../objects/Brachio.hpp"
#include "../objects/Skybox.h"
#include "../lighting/ShadowMap.h"
#include "../camera/Drone.h"


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

// ── Drone mode (added alongside fly-camera, not replacing it) ────────────────
// useDrone toggles which controller drives view+input each frame. False =
// the original fly-camera (W/S/A/D + arrow keys); true = the quadcopter
// drone (Report §8). The drone instance itself is declared in main() so it
// can be initialised after the GL context exists; the toggle and the mouse
// delta accumulators live here so the key + cursor callbacks can reach them.
static Drone* g_drone        = nullptr;
static bool   g_useDrone     = false;
static float  g_mouseAccumDx = 0.0f;
static float  g_mouseAccumDy = 0.0f;
static double g_lastMouseX   = 0.0;
static double g_lastMouseY   = 0.0;
static bool   g_firstMouse   = true;

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
    // Original fly-camera input. Only acts when drone mode is OFF — the
    // drone consumes the same A/D/W/S/Space/Shift keys in its own update(),
    // so we'd be double-handling them otherwise.
    if (g_useDrone) return;

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

// ─────────────────────────────────────────────────────────────────────────────
// GLFW key callback — toggles drone mode (C) and forwards V/Z (and any
// future one-off keys the drone cares about) to the drone instance.
// ─────────────────────────────────────────────────────────────────────────────
static void key_cb(GLFWwindow* /*w*/, int key, int /*scancode*/,
                   int action, int /*mods*/)
{
    // Forward to the drone so its V (1st/3rd-person) toggle works.
    if (g_drone) g_drone->onKey(key, action);

    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_C)
    {
        g_useDrone = !g_useDrone;
        // Reset mouse-delta accumulators so the new mode doesn't get a
        // sudden fling from the previous mode's accumulated drift.
        g_mouseAccumDx = 0.0f;
        g_mouseAccumDy = 0.0f;
        g_firstMouse   = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GLFW cursor callback — accumulates mouse deltas for the drone, since the
// drone's update() consumes a per-frame delta (not absolute position). The
// fly-camera ignores the mouse and uses arrow keys, so we don't drive
// yaw/pitch from here in fly-camera mode.
// ─────────────────────────────────────────────────────────────────────────────
static void cursor_cb(GLFWwindow* /*w*/, double x, double y)
{
    if (g_firstMouse)
    {
        g_lastMouseX = x;
        g_lastMouseY = y;
        g_firstMouse = false;
        return;
    }
    double dx = x - g_lastMouseX;
    double dy = y - g_lastMouseY;
    g_lastMouseX = x;
    g_lastMouseY = y;

    g_mouseAccumDx += (float)dx;
    g_mouseAccumDy += (float)dy;
}

int main()
{
    GLFWwindow* window;
    try { window = setUp(); }
    catch (const char* e) { std::cout << e << std::endl; throw; }

    // ── Input callbacks ──────────────────────────────────────────────────────
    // C toggles fly-camera <-> drone mode. The drone's V (1st/3rd-person)
    // and any future one-off keys it cares about are forwarded via onKey()
    // inside key_cb. The cursor is captured + hidden so the drone's mouse-
    // look gets continuous deltas instead of stopping at the screen edges.
    glfwSetKeyCallback(window, key_cb);
    glfwSetCursorPosCallback(window, cursor_cb);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    GLuint shader = LoadShaders("src/shaders/vertex.glsl", "src/shaders/fragment.glsl");

    // ── Volcano model + its own shader (added alongside the cone above) ─────
    GLuint volcanoShader = LoadShaders("src/shaders/volcano.vert", "src/shaders/volcano.frag");
    Volcano volcanoModel;
    volcanoModel.init();
    const glm::vec3 VOLCANO_POS(0.0f, 0.0f, -10.0f);
    const float     VOLCANO_SCALE = 3.33f;
    GLint volLocModel    = glGetUniformLocation(volcanoShader, "model");
    GLint volLocView     = glGetUniformLocation(volcanoShader, "view");
    GLint volLocProj     = glGetUniformLocation(volcanoShader, "projection");
    GLint volLocViewPos  = glGetUniformLocation(volcanoShader, "viewPos");
    GLint volLocLightDir = glGetUniformLocation(volcanoShader, "lightDir");
    GLint volLocLightCol = glGetUniformLocation(volcanoShader, "lightColor");
    GLint volLocAmbient  = glGetUniformLocation(volcanoShader, "ambientColor");
    GLint volLocTime     = glGetUniformLocation(volcanoShader, "time");

    // ── T-rex (own shader, vertex layout pos/normal/uv/color) ────────────────
    // Same pattern as the volcano: own shader program, own VAO inside the
    // class. Per-frame uniforms (camera, lighting) are uploaded in the
    // render loop. The trex header's modelMatrix() handles Blender +Z-up →
    // OpenGL +Y-up conversion + scale + yaw + translate.
    GLuint trexShader = LoadShaders("src/shaders/trex.vert", "src/shaders/trex.frag");
    Trex trex;
    trex.init();
    const glm::vec3 TREX_POS(35.0f, 0.0f, 0.0f);
    const float     TREX_SCALE = 0.5f;
    const float     TREX_YAW   = glm::radians(180.0f); // face -X (toward volcano)
    GLint trexLocModel    = glGetUniformLocation(trexShader, "model");
    GLint trexLocView     = glGetUniformLocation(trexShader, "view");
    GLint trexLocProj     = glGetUniformLocation(trexShader, "projection");
    GLint trexLocViewPos  = glGetUniformLocation(trexShader, "viewPos");
    GLint trexLocLightDir = glGetUniformLocation(trexShader, "lightDir");
    GLint trexLocLightCol = glGetUniformLocation(trexShader, "lightColor");
    GLint trexLocAmbient  = glGetUniformLocation(trexShader, "ambientColor");

    // ── Brachiosaurus (own shader, same vertex layout as trex) ───────────────
    GLuint brachioShader = LoadShaders("src/shaders/brachio.vert", "src/shaders/brachio.frag");
    Brachio brachio;
    brachio.init();
    const glm::vec3 BRACHIO_POS(-30.0f, 0.0f, 20.0f);
    const float     BRACHIO_SCALE = 0.4f;
    const float     BRACHIO_YAW   = glm::radians(90.0f); // face +X (toward volcano)
    GLint braLocModel    = glGetUniformLocation(brachioShader, "model");
    GLint braLocView     = glGetUniformLocation(brachioShader, "view");
    GLint braLocProj     = glGetUniformLocation(brachioShader, "projection");
    GLint braLocViewPos  = glGetUniformLocation(brachioShader, "viewPos");
    GLint braLocLightDir = glGetUniformLocation(brachioShader, "lightDir");
    GLint braLocLightCol = glGetUniformLocation(brachioShader, "lightColor");
    GLint braLocAmbient  = glGetUniformLocation(brachioShader, "ambientColor");

    GLint locLightColor = glGetUniformLocation(shader, "uLightColor");
    GLint locAmbient    = glGetUniformLocation(shader, "uAmbient");
    GLint locLightDir   = glGetUniformLocation(shader, "uLightDir");
    GLint locViewPos    = glGetUniformLocation(shader, "uViewPos");

    // ── Shadow uniforms on the main shader (Step 2) ──────────────────────────
    GLint locLightSpaceMat = glGetUniformLocation(shader, "uLightSpaceMatrix");
    GLint locShadowMap     = glGetUniformLocation(shader, "uShadowMap");
    GLint locHasShadowMap  = glGetUniformLocation(shader, "uHasShadowMap");

    // ── Volcano point light (a 2nd light source, not just ambient) ──────────
    // The position sits just above the crater rim so the warm glow reaches the
    // ground and nearby holes / dinosaurs. The light colour is a saturated
    // orange — multiplied here by an intensity scalar so the wattage is easy
    // to tweak without recolouring. Attenuation is in the shaders.
    const glm::vec3 VOLCANO_LIGHT_POS   = VOLCANO_POS + glm::vec3(0.0f, 12.0f, 0.0f);
    const glm::vec3 VOLCANO_LIGHT_COLOR = glm::vec3(1.0f, 0.45f, 0.10f) * 2.2f;

    // Uniform locations for the volcano light on EACH shader that should
    // receive it. Looked up once at startup. The uniform names are the
    // same across all four shaders for symmetry.
    GLint locVolPos       = glGetUniformLocation(shader,        "uVolcanoLightPos");
    GLint locVolCol       = glGetUniformLocation(shader,        "uVolcanoLightColor");
    GLint volLocVolPos    = glGetUniformLocation(volcanoShader, "uVolcanoLightPos");
    GLint volLocVolCol    = glGetUniformLocation(volcanoShader, "uVolcanoLightColor");
    GLint trexLocVolPos   = glGetUniformLocation(trexShader,    "uVolcanoLightPos");
    GLint trexLocVolCol   = glGetUniformLocation(trexShader,    "uVolcanoLightColor");
    GLint braLocVolPos    = glGetUniformLocation(brachioShader, "uVolcanoLightPos");
    GLint braLocVolCol    = glGetUniformLocation(brachioShader, "uVolcanoLightColor");

    // ── Skybox (day/night cycle drives the lighting) ─────────────────────────
    // The skybox rotates a sun + moon around the world X axis. Whichever is
    // currently above the horizon drives the directional light for every
    // hole, the volcano, the ground. The skybox itself is drawn LAST each
    // frame with GL_LEQUAL so it sits behind everything.
    Skybox skybox;
    skybox.dayLengthSecs = 120.0f;  // 2 minutes per full cycle
    skybox.init("src/shaders/skybox.vert",
                "src/shaders/skybox.frag",
                "src/assets/textures/sky_day.bmp",
                "src/assets/textures/sky_night.bmp");

    // ── Shadow map (depth pass — Step 1 of shadow mapping integration) ───────
    // For now, the depth pass runs and populates the depth texture every
    // frame, but the main fragment shader doesn't sample from it yet. Step 2
    // wires the depth texture into vertex.glsl + fragment.glsl so the
    // shadows actually appear. setSceneBounds() controls the orthographic
    // frustum that captures shadow casters — a 250-unit radius around the
    // origin covers the whole course with room to spare.
    ShadowMap shadowMap;
    if (!shadowMap.init(2048,
                        "src/shaders/shadow.vert",
                        "src/shaders/shadow.frag")) {
        std::cerr << "ShadowMap init failed — depth pass will be skipped.\n";
    }
    shadowMap.setSceneBounds(glm::vec3(0.0f, 0.0f, 0.0f), 250.0f);

    // ── Drone (Report §8) ────────────────────────────────────────────────────
    // Created here so the GL context exists. Shares the main shader program
    // — Drone::init binds the uniform locations and uses the same
    // vertex-attribute layout as the holes (pos/normal/colour). Its
    // global pointer (used by the key+cursor callbacks) is set so input
    // forwarding works as soon as the drone is constructed.
    Drone drone;
    drone.init(shader);
    g_drone = &drone;

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

        // ── Live framebuffer size ────────────────────────────────────────────
        // The shadow depth pass changes the viewport mid-frame (to the size
        // of the shadow map), so we MUST reset the viewport back to the real
        // framebuffer size before any colour-pass draws. Querying GLFW every
        // frame also makes the program survive window resizes gracefully —
        // the perspective projection is recomputed from the live aspect.
        int fbW = WIDTH, fbH = HEIGHT;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbH == 0) fbH = 1;   // guard against minimised window
        float aspect = (float)fbW / (float)fbH;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ── Frame timing (top of loop so skybox + water animations all see
        //    the same dt; previously this was buried mid-loop). ────────────────
        static double lastTime = glfwGetTime();
        double now      = glfwGetTime();
        float deltaTime = (float)(now - lastTime);
        lastTime = now;

        // ── Drone update ─────────────────────────────────────────────────────
        // Always called so the drone keeps spinning its propellers even when
        // the fly-camera is active (it costs nothing). It reads keyboard
        // state from `window` directly. In drone mode the accumulated mouse
        // deltas drive 1st-person look; in fly-camera mode the deltas are
        // ignored (the fly camera uses arrow keys for look).
        drone.update(window, deltaTime,
                     g_useDrone ? g_mouseAccumDx : 0.0f,
                     g_useDrone ? g_mouseAccumDy : 0.0f);
        g_mouseAccumDx = 0.0f;
        g_mouseAccumDy = 0.0f;

        // Projection comes from the drone in drone mode (its own FoV +
        // optional Z-zoom), or our standard 45° perspective in fly-camera.
        if (g_useDrone) {
            projection = drone.projection(aspect);
        } else {
            projection = glm::perspective(
                glm::radians(45.0f), aspect, 0.1f, 500.0f);
        }

        // ── Skybox: advance the rotation, then read sun / moon. ──────────────
        skybox.update(deltaTime);
        SkyLight sun  = skybox.getSun();
        SkyLight moon = skybox.getMoon();

        // Pick whichever of sun / moon is above the horizon and stronger.
        // The moon's intensity is 0.25 * t in Skybox.cpp so multiply by 4 when
        // comparing against the sun.
        glm::vec3 sunDir;
        if (sun.intensity >= moon.intensity * 4.0f && glm::length(sun.position) > 1e-3f) {
            sunDir = glm::normalize(sun.position);
        } else if (glm::length(moon.position) > 1e-3f) {
            sunDir = glm::normalize(moon.position);
        } else {
            sunDir = glm::normalize(glm::vec3(0.45f, 0.80f, 0.35f));
        }
        // Horizon clamp: never let the directional light come from below the
        // ground, otherwise every up-facing surface goes pitch black.
        if (sunDir.y < 0.10f) {
            sunDir.y = 0.10f;
            sunDir = glm::normalize(sunDir);
        }

        // Blend warm-day light vs cool-moon light by sun intensity.
        float dayBlend = glm::clamp(sun.intensity, 0.0f, 1.0f);
        glm::vec3 dayLight  (1.00f, 0.97f, 0.90f);
        glm::vec3 nightLight(0.45f, 0.55f, 0.75f);
        glm::vec3 sunColor = glm::mix(nightLight, dayLight, dayBlend);

        // Ambient: brighter, warmer during the day; dimmer, cooler at night.
        glm::vec3 dayAmbient  (0.32f, 0.30f, 0.26f);
        glm::vec3 nightAmbient(0.08f, 0.10f, 0.16f);
        glm::vec3 ambient = glm::mix(nightAmbient, dayAmbient, dayBlend);

        // ── Shadow map depth pass ────────────────────────────────────────────
        // Renders every shadow caster from the sun's POV into the shadow map.
        // The colour pass below isn't sampling from it yet — that's Step 2 of
        // the shadow integration. For now this just exercises the depth path
        // so we catch any wiring errors before touching the main shader.
        shadowMap.beginPass(sunDir);
        // Caller responsibility: every renderable that should cast a shadow
        // must implement drawDepth(GLuint) and be invoked here. Water meshes
        // (h16 stream/moat, h18 lakes/stream) are skipped inside each hole's
        // drawDepth. The Volcano model gets its drawDepth in Step 3.
        h1.drawDepth (shadowMap.depthShader());
        h2.drawDepth (shadowMap.depthShader());
        h3.drawDepth (shadowMap.depthShader());
        h4.drawDepth (shadowMap.depthShader());
        h5.drawDepth (shadowMap.depthShader());
        h6.drawDepth (shadowMap.depthShader());
        h7.drawDepth (shadowMap.depthShader());
        h8.drawDepth (shadowMap.depthShader());
        h9e.drawDepth(shadowMap.depthShader());
        h9w.drawDepth(shadowMap.depthShader());
        h10.drawDepth(shadowMap.depthShader());
        h11.drawDepth(shadowMap.depthShader());
        h12.drawDepth(shadowMap.depthShader());
        h13.drawDepth(shadowMap.depthShader());
        h14.drawDepth(shadowMap.depthShader());
        h15.drawDepth(shadowMap.depthShader());
        h16.drawDepth(shadowMap.depthShader());
        h17.drawDepth(shadowMap.depthShader());
        h18.drawDepth(shadowMap.depthShader());
        // Ground is a single Mesh — call its drawDepth directly.
        ground.drawDepth(shadowMap.depthShader());
        // Drone — only casts when in 3rd-person mode (drone.drawDepth has
        // an mThirdPerson guard, so calling it unconditionally is safe).
        drone.drawDepth(shadowMap.depthShader());
        // External Blender-imported models — same depth-shader pattern.
        trex.drawDepth   (shadowMap.depthShader(), TREX_POS,    TREX_SCALE,    TREX_YAW);
        brachio.drawDepth(shadowMap.depthShader(), BRACHIO_POS, BRACHIO_SCALE, BRACHIO_YAW);
        shadowMap.endPass(fbW, fbH);

        // ── Pick view + eye position based on active controller ──────────────
        // In drone mode the drone owns the camera (Report §8.3 / §8.3.1).
        // In fly-camera mode we use the original lookAt-driven matrix.
        glm::mat4 view;
        glm::vec3 eyePos;
        if (g_useDrone) {
            view   = drone.view();
            eyePos = drone.eyePos();
        } else {
            view   = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            eyePos = cameraPos;
        }

        glUseProgram(shader);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        // Per-frame light uniforms now driven by the skybox sun/moon cycle
        // instead of being hardcoded. Any hole that doesn't pass its own
        // light direction (e.g. h16 / h17 / h18) reads these.
        glUniform3fv(locLightColor, 1, glm::value_ptr(sunColor));
        glUniform3fv(locAmbient,    1, glm::value_ptr(ambient));
        if (locLightDir != -1) glUniform3fv(locLightDir, 1, glm::value_ptr(sunDir));
        if (locViewPos  != -1) glUniform3fv(locViewPos,  1, glm::value_ptr(eyePos));

        // ── Shadow uniforms on the main shader ───────────────────────────────
        // Bind the depth texture from the shadow pre-pass to texture unit 1
        // (unit 0 is reserved for any future colour texture). Tell the
        // shader to sample from it, and pass the same light-space matrix the
        // depth pass used so projected positions line up.
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTexture());
        if (locShadowMap != -1) glUniform1i(locShadowMap, 1);
        if (locHasShadowMap != -1) glUniform1i(locHasShadowMap, 1);
        if (locLightSpaceMat != -1)
            glUniformMatrix4fv(locLightSpaceMat, 1, GL_FALSE,
                               glm::value_ptr(shadowMap.lightSpaceMatrix()));
        glActiveTexture(GL_TEXTURE0);   // restore default unit for any code below
                                        // that does glBindTexture without setting active

        // ── Volcano point light on the main shader ──────────────────────────
        // Uploaded once per frame (constant for now — could be modulated by
        // dayBlend later to make the volcano dominate at night).
        if (locVolPos != -1) glUniform3fv(locVolPos, 1, glm::value_ptr(VOLCANO_LIGHT_POS));
        if (locVolCol != -1) glUniform3fv(locVolCol, 1, glm::value_ptr(VOLCANO_LIGHT_COLOR));

        ground.draw(shader, false);

        // ── Volcano model draw (own shader) ──────────────────────────────────
        glUseProgram(volcanoShader);
        glUniformMatrix4fv(volLocProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(volLocView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(volLocViewPos, 1, glm::value_ptr(eyePos));
        // Skybox-driven lighting (computed at top of frame).
        glUniform3fv(volLocLightDir, 1, glm::value_ptr(sunDir));
        glUniform3fv(volLocLightCol, 1, glm::value_ptr(sunColor));
        glUniform3fv(volLocAmbient,  1, glm::value_ptr(ambient));
        glUniform1f(volLocTime, (float)glfwGetTime());
        if (volLocVolPos != -1) glUniform3fv(volLocVolPos, 1, glm::value_ptr(VOLCANO_LIGHT_POS));
        if (volLocVolCol != -1) glUniform3fv(volLocVolCol, 1, glm::value_ptr(VOLCANO_LIGHT_COLOR));
        volcanoModel.draw(volLocModel, VOLCANO_POS, VOLCANO_SCALE);

        // ── T-rex (own shader) ───────────────────────────────────────────────
        // Same pattern as volcano: switch program, upload per-frame uniforms,
        // draw, switch back to main shader for the hole draws below.
        glUseProgram(trexShader);
        glUniformMatrix4fv(trexLocProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(trexLocView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(trexLocViewPos,  1, glm::value_ptr(eyePos));
        glUniform3fv(trexLocLightDir, 1, glm::value_ptr(sunDir));
        glUniform3fv(trexLocLightCol, 1, glm::value_ptr(sunColor));
        glUniform3fv(trexLocAmbient,  1, glm::value_ptr(ambient));
        if (trexLocVolPos != -1) glUniform3fv(trexLocVolPos, 1, glm::value_ptr(VOLCANO_LIGHT_POS));
        if (trexLocVolCol != -1) glUniform3fv(trexLocVolCol, 1, glm::value_ptr(VOLCANO_LIGHT_COLOR));
        trex.draw(trexLocModel, TREX_POS, TREX_SCALE, TREX_YAW);

        // ── Brachiosaurus (own shader) ───────────────────────────────────────
        glUseProgram(brachioShader);
        glUniformMatrix4fv(braLocProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(braLocView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(braLocViewPos,  1, glm::value_ptr(eyePos));
        glUniform3fv(braLocLightDir, 1, glm::value_ptr(sunDir));
        glUniform3fv(braLocLightCol, 1, glm::value_ptr(sunColor));
        glUniform3fv(braLocAmbient,  1, glm::value_ptr(ambient));
        if (braLocVolPos != -1) glUniform3fv(braLocVolPos, 1, glm::value_ptr(VOLCANO_LIGHT_POS));
        if (braLocVolCol != -1) glUniform3fv(braLocVolCol, 1, glm::value_ptr(VOLCANO_LIGHT_COLOR));
        brachio.draw(braLocModel, BRACHIO_POS, BRACHIO_SCALE, BRACHIO_YAW);

        glUseProgram(shader);

        h1.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                      // camera position

        h2.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h3.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h4.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h5.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h6.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h7.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h8.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h9e.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h9w.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position


        h10.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h11.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h12.draw(shader, false,
        sunDir,  // light direction
        sunColor,                 // light colour
        eyePos);                                     //camera position

        h13.draw(view, projection,
         sunDir,  // sunDir
         eyePos); 
         
         h14.draw(view, projection,
         sunDir,  // sunDir
         eyePos); 
         
        h15.draw(view, projection,
         sunDir,  // sunDir
         eyePos);   


        h16.update(deltaTime);
        h16.setCameraData(view, projection, eyePos);
        h16.draw(shader, false);

        h17.draw(shader, false);


        h18.update(deltaTime);
        h18.setCameraData(view, projection, eyePos);
        h18.draw(shader, false);

        // ── Drone (3rd-person body) ──────────────────────────────────────────
        // drone.draw no-ops in 1st-person, so calling it unconditionally is
        // safe and keeps the program-wide uniforms in scope for the skybox
        // draw immediately below.
        drone.draw(view, projection, sunDir, eyePos);

        // ── Skybox last (sits behind everything via depth-test trick) ────────
        // Skybox.cpp internally disables depth writes + culling and restores
        // them, so we just need to switch the depth func so it can draw at
        // the far plane behind anything already rendered.
        glDepthFunc(GL_LEQUAL);
        skybox.draw(view, projection);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    volcanoModel.cleanup();
    trex.cleanup();
    brachio.cleanup();
    drone.cleanup();
    glfwTerminate();
    return 0;
}