// =============================================================================
// dev.cpp
// Standalone viewer for COS344 mini-golf course holes.
//
// Hosts the team's hole renderers in one viewer with two camera modes and a
// shared dynamic skybox driving the time-of-day lighting.
//
// Holes:
//   1 / 2 / 3      - Ayush's Hole 13 / 14 / 15
//   4              - Shav's   Hole 16
//   5 / 6 / 7 / 8  - Diya's  Hole 5 / 6 / 7 / 8
//   9 / 10 / 11    - Hole 10 / 11 / 12
//   F1/F2/F3/F4    - Hole 1 / 2 / 3 / 4
//
// All Mesh-based holes (5/6/7/8/10/11/12/16) render through the SAME shader as the
// per-vertex-colour holes (13/14/15) — Mesh.cpp feeds the per-mesh r/g/b
// to the shader via a constant vertex attribute, so the procedural turf/
// kerb classifier in fragment.glsl picks the right look automatically.
//
// The Skybox runs a slow day/night cycle (sphere rotating around the world
// X axis). Its sun/moon position is fed into BOTH shader programs each frame
// as the directional light direction, so the lighting on every hole tracks
// the visible sky. The skybox itself is drawn once per frame after the
// scene, using the depth-trick (gl_Position.xyww + GL_LEQUAL).
//
// Camera modes:
//   1. FLY-CAMERA (default) — the original free-flight camera.
//   2. DRONE (press C) — the quadcopter from Report §8.
// Both controllers coexist; C toggles which one drives the view+input.
//
// Controls:
//   1 / 2 / 3 / 4 / 5 / 6 / 7 / 8 / 9 / 0 / -   switch hole (13/14/15/16/5/6/7/8/10/11/12)
//   F1 / F2 / F3 / F4                             switch hole (1/2/3/4)
//   C              toggle FLY-CAMERA <-> DRONE
//   R              re-frame the active controller on the current hole
//   B              toggle the volcano landmark on/off
//   N              toggle the T-rex landmark on/off
//   G              toggle the Brachiosaurus landmark on/off
//
//   Fly-camera (default):
//     W / S        fly forward / back
//     A / D        strafe left / right
//     SPACE/LSHIFT ascend / descend
//     Mouse        look around
//
//   Drone (after pressing C):
//     W / S        pitch + translate forward / back (Report §8.2)
//     A / D        yaw left / right
//     Q / E        roll left / right
//     SPACE/LSHIFT ascend / descend
//     V            toggle 1st-person <-> 3rd-person view
//     Z (hold)     zoom in (FoV 60° → 20°)
//     Mouse        camera look (1st-person only; locked in 3rd-person)
//
//   TAB            toggle wireframe
//   M              release / re-capture the mouse cursor
//   ESC            quit
//
// Only spec-allowed libraries are used. All shaders live under src/shaders/
// as .glsl files and are loaded from disk at startup.
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
// ADD HOLE 15: when that class exists, add its #include here.
#include "../objects/Hole15.h"

// Shav's contributions — Hole 16 + the dynamic skybox + the Mesh class his
// holes are built from. These live in the same src/ tree as our holes, so
// the relative includes match the existing pattern.
#include "../core/Mesh.h"
#include "../objects/Hole16.h"
#include "../objects/Skybox.h"
// Diya's holes — same Mesh-based pattern as Hole16 (raw Mesh* array,
// translate / draw API). They get wired in via the dev.cpp render switch
// the same way Hole16 does.
#include "../objects/Hole5.h"
#include "../objects/Hole6.h"
#include "../objects/Hole7.h"
#include "../objects/Hole8.h"
#include "../objects/Hole10.h"
#include "../objects/Hole11.h"
#include "../objects/Hole12.h"
// Holes 1 / 2 / 3 / 4 — same Mesh-based pattern as holes 5–8.
#include "../objects/Hole1.h"
#include "../objects/Hole2.h"
#include "../objects/Hole3.h"
#include "../objects/Hole4.h"
// Holes 17 / 18 — same Mesh-based pattern. Hole17 is the cave; Hole18 is
// the S-shaped river hole with animated water (uses WaterMesh internally,
// like Hole16).
#include "../objects/Hole17.h"
#include "../objects/Hole18.h"
// Hole 9 — split into West approach (with cup/flag + tunnel entry) and
// East exit (tunnel mouth + run-out fairway). Both follow the Mesh-based
// pattern of Holes 5–8.
#include "../objects/Hole9W.h"
#include "../objects/Hole9E.h"

#include "../camera/Drone.h"

// Volcano — header-only decorative obstacle modelled in Blender, baked into
// volcano_mesh.h as raw vertex/index arrays. Rendered with its OWN shader
// pair so the lava streaks shade as emissive instead of being repainted by
// the hole/turf classifier in fragment.glsl. Toggled per-hole via the B key.
#include "../objects/Volcano.hpp"

// T-rex — same pattern as the volcano. Detailed reptilian decoration baked
// into trex_mesh.h. Its own shader pair handles the green-skin/teeth/claws
// vertex colours without going through the hole turf classifier (which
// would mistakenly treat the green skin as turf). Toggled via the N key.
#include "../objects/Trex.hpp"

// Brachiosaurus — same pipeline. Long-necked sauropod landmark for the
// "lost-world" backdrop. Dedicated shader because its earthy skin would
// also get misclassified by fragment.glsl. Toggled via the G key.
#include "../objects/Brachio.hpp"


// -----------------------------------------------------------------------------
// Shader source paths. ONE shader program drives every renderable in the
// scene (Holes 13/14/15/16 + Drone), so a uniform set by dev.cpp on this
// program is visible to all of them. The Skybox has its own shader pair —
// it draws with a completely separate program created by Skybox::init().
//
// Paths are relative to the working directory the program is launched from.
// Run from the repo root so "src/shaders/..." resolves correctly.
// -----------------------------------------------------------------------------
static const char* VERTEX_PATH       = "src/shaders/vertex.glsl";
static const char* FRAGMENT_PATH     = "src/shaders/fragment.glsl";
static const char* SKYBOX_VERT_PATH  = "src/shaders/skybox.vert";
static const char* SKYBOX_FRAG_PATH  = "src/shaders/skybox.frag";
// Volcano runs on its OWN shader pair, separate from the hole/turf shader.
// The hole shader's fragment.glsl is built around the procedural turf/kerb
// classifier (it picks a material from per-mesh r/g/b), which would silently
// repaint the volcano's rock + lava colours. Volcano keeps a dedicated
// Phong+emissive shader so the lava actually glows.
static const char* VOLCANO_VERT_PATH = "src/shaders/volcano.vert";
static const char* VOLCANO_FRAG_PATH = "src/shaders/volcano.frag";
// T-rex shader pair. Same reasoning as the volcano: its baked-in green skin
// would get repainted by the hole shader's turf classifier (greens get the
// procedural turf treatment), and its teeth/claws/eyes need a different
// specular profile. Dedicated trex.vert/trex.frag keeps it correct.
static const char* TREX_VERT_PATH    = "src/shaders/trex.vert";
static const char* TREX_FRAG_PATH    = "src/shaders/trex.frag";
// Brachiosaurus shader pair. Earthy sauropod skin + dark nails; same
// lightDir convention as fragment.glsl.
static const char* BRACHIO_VERT_PATH = "src/shaders/brachio.vert";
static const char* BRACHIO_FRAG_PATH = "src/shaders/brachio.frag";

// Read an entire file into a std::string. Uses C stdio so we only depend on
// headers the assignment spec explicitly allows (<stdio.h>, <sstream>).
// Returns an empty string on failure and logs the reason to stderr.
static std::string readFile(const char* path)
{
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return std::string();
    }
    std::fseek(f, 0, SEEK_END);
    long len = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (len <= 0) {
        std::fclose(f);
        std::cerr << "Shader file is empty: " << path << std::endl;
        return std::string();
    }
    std::string out;
    out.resize((size_t)len);
    size_t got = std::fread(&out[0], 1, (size_t)len, f);
    std::fclose(f);
    if (got != (size_t)len) {
        std::cerr << "Short read on shader file: " << path << std::endl;
        return std::string();
    }
    return out;
}

static GLuint compileShader(GLenum type, const char* src, const char* label) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, 2048, NULL, log);
        std::cerr << "Shader compile error in " << label << ":\n"
                  << log << std::endl;
    }
    return s;
}

// Build one shader program from a vertex + fragment GLSL file pair.
// Returns 0 on failure (with diagnostics already logged to stderr).
static GLuint buildProgramFromFiles(const char* vertPath, const char* fragPath)
{
    std::string vsSrc = readFile(vertPath);
    std::string fsSrc = readFile(fragPath);
    if (vsSrc.empty() || fsSrc.empty()) {
        std::cerr << "Cannot build shader program — source files missing ("
                  << vertPath << ", " << fragPath << ")." << std::endl;
        return 0;
    }
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vsSrc.c_str(), vertPath);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str(), fragPath);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, 2048, NULL, log);
        std::cerr << "Program link error (" << vertPath << " + "
                  << fragPath << "):\n" << log << std::endl;
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

// ── Drone (added alongside the fly-camera, not replacing it) ─────────────────
// g_useDrone toggles which one drives the view + input. False = the original
// fly-camera; true = the quadcopter drone from Report §8. Toggle with C.
// Mouse deltas are accumulated by cursor_cb each time the OS reports cursor
// movement, then consumed once per frame by either the camera (subtracted
// directly from g_cam.yaw/pitch — the legacy path) or the drone (consumed
// via g_drone.update()'s mouseDx/Dy parameters). Tracking deltas in one
// place keeps the two input paths from fighting over the cursor.
static Drone  g_drone;
static bool   g_useDrone     = false;
static float  g_mouseAccumDx = 0.0f;
static float  g_mouseAccumDy = 0.0f;

// Forward declarations of the hole instances so callbacks can reference them.
static Hole13* g_h13 = NULL;
static Hole14* g_h14 = NULL;
// ADD HOLE 15: declare g_h15 pointer here when that class exists.
static Hole15* g_h15 = NULL;

// Shav's Hole 16 + the dynamic skybox. The skybox is shared by ALL holes —
// its current sun/moon direction is fed into the shader each frame as the
// directional light, so 13/14/15/16 all track the same time of day.
static Hole16* g_h16     = NULL;
// Diya's Mesh-based holes. Same pattern as Hole16.
static Hole5*  g_h5  = NULL;
static Hole6*  g_h6  = NULL;
static Hole7*  g_h7  = NULL;
static Hole8*  g_h8  = NULL;
// Holes 10 / 11 / 12 — same Mesh-based pattern.
static Hole10* g_h10 = NULL;
static Hole11* g_h11 = NULL;
static Hole12* g_h12 = NULL;
// Holes 1 / 2 / 3 / 4 — same Mesh-based pattern.
static Hole1*  g_h1  = NULL;
static Hole2*  g_h2  = NULL;
static Hole3*  g_h3  = NULL;
static Hole4*  g_h4  = NULL;
// Holes 17 / 18 — same Mesh-based pattern. Hole18 has water like Hole16.
static Hole17* g_h17 = NULL;
static Hole18* g_h18 = NULL;
// Hole 9 — split West / East halves.
static Hole9W* g_h9W = NULL;
static Hole9E* g_h9E = NULL;
static Skybox  g_skybox;

// Volcano decoration. One instance shared across every hole; B toggles its
// visibility. g_volcanoProgram is the dedicated Phong+emissive shader pair
// loaded from volcano.vert / volcano.frag in main(). The volcano is placed
// at a fixed offset behind/right of the active hole's centre so it shows up
// as a far-background landmark rather than blocking the fairway.
static Volcano g_volcano;
static GLuint  g_volcanoProgram = 0;
static bool    g_showVolcano    = true;

// T-rex decoration. Same pattern as the volcano. Dedicated shader program
// so the skin/teeth/claws colours render with the right Phong tweaks
// instead of being misclassified as turf. Toggled via the N key.
static Trex    g_trex;
static GLuint  g_trexProgram = 0;
static bool    g_showTrex    = true;

// Brachiosaurus decoration. Same pattern. Toggled via the G key.
static Brachio g_brachio;
static GLuint  g_brachioProgram = 0;
static bool    g_showBrachio    = true;

static void frameCurrent() {
    glm::vec3 c(0.0f); float r = 50.0f;
    if (g_activeHole == 13 && g_h13) { c = g_h13->sceneCenter(); r = g_h13->sceneRadius(); }
    if (g_activeHole == 14 && g_h14) { c = g_h14->sceneCenter(); r = g_h14->sceneRadius(); }
    // ADD HOLE 15: add the matching `if` line here.
    if (g_activeHole == 15 && g_h15) { c = g_h15->sceneCenter(); r = g_h15->sceneRadius(); }
    // Hole 16 doesn't expose sceneCenter/sceneRadius — its arc fairway is
    // centred at the origin with outer radius ~19 m and ground disc ~21 m,
    // so a 30 m framing radius gives a comfortable view of everything.
    if (g_activeHole == 16 && g_h16) { c = glm::vec3(0.0f, 0.5f, 0.0f); r = 30.0f; }
    // Diya's holes: each is 28 (X) × 15 (Z) and was translated to a specific
    // world position in friend's dev.cpp. Match those positions here so R
    // reframes onto the visible hole rather than back to the origin.
    if (g_activeHole == 5  && g_h5)  { c = glm::vec3(-42.0f, 0.5f, -22.5f); r = 22.0f; }
    if (g_activeHole == 6  && g_h6)  { c = glm::vec3(-14.0f, 0.5f,  -7.5f); r = 22.0f; }
    if (g_activeHole == 7  && g_h7)  { c = glm::vec3( 14.0f, 0.5f,   7.5f); r = 22.0f; }
    if (g_activeHole == 8  && g_h8)  { c = glm::vec3( 42.0f, 0.5f,  22.5f); r = 22.0f; }
    // Holes 10 / 11 / 12: centred at their world translation origin.
    // Hole10 is 48 units long; framing radius 35 gives a comfortable overview.
    if (g_activeHole == 10 && g_h10) { c = glm::vec3(  0.0f, 2.0f,   0.0f); r = 35.0f; }
    // Hole11 L-shape spans ~28 along X then ~28 along Z; 30 covers the corner.
    if (g_activeHole == 11 && g_h11) { c = glm::vec3(  0.0f, 1.0f,  14.0f); r = 30.0f; }
    // Hole12 hourglass is 28 units long; 22 shows both wide ends.
    if (g_activeHole == 12 && g_h12) { c = glm::vec3(  0.0f, 0.5f,   0.0f); r = 22.0f; }
    // Holes 1 / 2 / 3 / 4 — built at local origin, 28×15 footprint.
    if (g_activeHole == 1  && g_h1)  { c = glm::vec3(  0.0f, 1.0f,   0.0f); r = 22.0f; }
    if (g_activeHole == 2  && g_h2)  { c = glm::vec3(  0.0f, 1.0f,   0.0f); r = 22.0f; }
    if (g_activeHole == 3  && g_h3)  { c = glm::vec3(  0.0f, 2.0f,   0.0f); r = 22.0f; }
    // Hole4 (windmill) is on a raised hill; frame a bit higher and wider.
    if (g_activeHole == 4  && g_h4)  { c = glm::vec3(  0.0f, 3.0f,   0.0f); r = 26.0f; }
    // Hole 17 — cave hole running along +X. Geometry sits between x≈0
    // (tee) and x≈28 (green). Centre at the cave's midpoint with enough
    // radius to fit the rock dome on top.
    if (g_activeHole == 17 && g_h17) { c = glm::vec3( 14.0f, 1.0f,   0.0f); r = 24.0f; }
    // Hole 18 — S-shaped river hole. Tee at (13, ?, 0), green at (12, ?, 40)
    // so the bounding box centre is roughly (12, 0, 20); a 28 m radius
    // shows both curves and the lake on the right.
    if (g_activeHole == 18 && g_h18) { c = glm::vec3( 12.0f, 1.0f,  20.0f); r = 28.0f; }
    // Hole 9 — West (approach + cup/flag/tunnel-entry) and East (tunnel-exit
    // + run-out). Both 28×15, built at the local origin. We position them
    // side-by-side along X (W on -X side, E on +X side) with a small gap.
    if (g_activeHole == 91 && g_h9W) { c = glm::vec3(-15.0f, 0.5f,   0.0f); r = 22.0f; }
    if (g_activeHole == 92 && g_h9E) { c = glm::vec3( 15.0f, 0.5f,   0.0f); r = 22.0f; }
    g_cam.frameOn(c, r);
    g_drone.frameOn(c, r);   // also reframe the drone so R works in either mode
}

static void key_cb(GLFWwindow* w, int key, int /*sc*/, int action, int /*mods*/) {
    // Forward keys to the drone so it can pick up V (third-person toggle)
    // and any other one-off keys it cares about. The drone's onKey ignores
    // anything it doesn't recognise, so this is safe regardless of mode.
    g_drone.onKey(key, action);

    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, GLFW_TRUE);
    if (key == GLFW_KEY_1) { g_activeHole = 13; g_needFrame = true; }
    if (key == GLFW_KEY_2) { g_activeHole = 14; g_needFrame = true; }
    // ADD HOLE 15: bind key 3 here.
    if (key == GLFW_KEY_3) { g_activeHole = 15; g_needFrame = true; }
    if (key == GLFW_KEY_4) { g_activeHole = 16; g_needFrame = true; }
    // Diya's holes 5–8 take the number keys that match their hole number,
    // for memorability. Drone-toggle, wireframe etc. stay on letter keys.
    if (key == GLFW_KEY_5) { g_activeHole = 5;  g_needFrame = true; }
    if (key == GLFW_KEY_6) { g_activeHole = 6;  g_needFrame = true; }
    if (key == GLFW_KEY_7) { g_activeHole = 7;  g_needFrame = true; }
    if (key == GLFW_KEY_8) { g_activeHole = 8;  g_needFrame = true; }
    // Holes 10 / 11 / 12 on keys 9 / 0 / minus.
    if (key == GLFW_KEY_9)     { g_activeHole = 10; g_needFrame = true; }
    if (key == GLFW_KEY_0)     { g_activeHole = 11; g_needFrame = true; }
    if (key == GLFW_KEY_MINUS) { g_activeHole = 12; g_needFrame = true; }
    // Holes 1 / 2 / 3 / 4 on F1 / F2 / F3 / F4 (keys 1-4 are taken by holes 13-16).
    if (key == GLFW_KEY_F1) { g_activeHole = 1; g_needFrame = true; }
    if (key == GLFW_KEY_F2) { g_activeHole = 2; g_needFrame = true; }
    if (key == GLFW_KEY_F3) { g_activeHole = 3; g_needFrame = true; }
    if (key == GLFW_KEY_F4) { g_activeHole = 4; g_needFrame = true; }
    // Holes 17 / 18 on the bracket keys [ and ] — physically adjacent on
    // the keyboard so they're easy to remember as "the last two".
    if (key == GLFW_KEY_LEFT_BRACKET)  { g_activeHole = 17; g_needFrame = true; }
    if (key == GLFW_KEY_RIGHT_BRACKET) { g_activeHole = 18; g_needFrame = true; }
    // Hole 9 — semicolon = West half (approach + flag), apostrophe = East
    // half (tunnel exit). Adjacent keys to mirror the two halves of the hole.
    if (key == GLFW_KEY_SEMICOLON)  { g_activeHole = 91; g_needFrame = true; }
    if (key == GLFW_KEY_APOSTROPHE) { g_activeHole = 92; g_needFrame = true; }
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
    // ── B: toggle the volcano landmark on/off ────────────────────────────────
    // Not bound to V (drone 1st/3rd-person) or Z (drone zoom) to avoid
    // clashing with the drone's existing controls. B is otherwise unused.
    if (key == GLFW_KEY_B) {
        g_showVolcano = !g_showVolcano;
        std::cout << "Volcano: " << (g_showVolcano ? "ON" : "OFF") << std::endl;
    }

    // ── N: toggle the T-rex landmark on/off ──────────────────────────────────
    // Picked N because every other letter row near here is taken: C drone
    // toggle, V drone view, B volcano, M mouse, R reframe. N has no clash.
    if (key == GLFW_KEY_N) {
        g_showTrex = !g_showTrex;
        std::cout << "T-rex: " << (g_showTrex ? "ON" : "OFF") << std::endl;
    }

    // ── G: toggle the brachiosaurus landmark on/off ──────────────────────────
    // G for "giant" / "grazer". Free key.
    if (key == GLFW_KEY_G) {
        g_showBrachio = !g_showBrachio;
        std::cout << "Brachiosaurus: " << (g_showBrachio ? "ON" : "OFF") << std::endl;
    }

    // ── C: toggle fly-camera ↔ drone mode ────────────────────────────────────
    if (key == GLFW_KEY_C) {
        g_useDrone = !g_useDrone;
        // Reset the mouse delta accumulator so a big jump from the previous
        // mode's cursor position doesn't suddenly fling the new view.
        g_mouseAccumDx = 0.0f;
        g_mouseAccumDy = 0.0f;
        g_firstMouse = true;
    }
}

static void cursor_cb(GLFWwindow* /*w*/, double x, double y) {
    if (!g_mouseLook) return;
    if (g_firstMouse) { g_lastX = x; g_lastY = y; g_firstMouse = false; return; }
    double dx = x - g_lastX, dy = y - g_lastY;
    g_lastX = x; g_lastY = y;

    // Accumulate raw pixel deltas for the drone (consumed once per frame
    // by g_drone.update()). This is independent of the camera path below
    // so neither mode ever drops mouse input.
    g_mouseAccumDx += (float)dx;
    g_mouseAccumDy += (float)dy;

    // Original fly-camera path — only active when the camera is driving.
    if (!g_useDrone) {
        const float sens = 0.0025f;
        g_cam.yaw   += (float)dx * sens;
        g_cam.pitch -= (float)dy * sens;
        if (g_cam.pitch >  1.5f) g_cam.pitch =  1.5f;
        if (g_cam.pitch < -1.5f) g_cam.pitch = -1.5f;
    }
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
        "COS344 - Holes 13/14/15 (1/2/3 select, C toggle drone, R reframe)", NULL, NULL);
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
    // Back-face culling is disabled: the hole geometry uses mixed winding on
    // side/kerb faces, so culling silently drops valid faces and leaves large
    // dark patches. For a top-down mini-golf viewer the overdraw cost is fine.
    glDisable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    GLuint program = buildProgramFromFiles(VERTEX_PATH, FRAGMENT_PATH);

    // Dedicated volcano shader. Reused for every hole because the volcano is
    // a shared decoration. If the source files are missing, buildProgramFromFiles
    // returns 0 and we just skip volcano rendering (volcano stays invisible).
    g_volcanoProgram = buildProgramFromFiles(VOLCANO_VERT_PATH, VOLCANO_FRAG_PATH);
    if (g_volcanoProgram == 0) {
        std::cerr << "Volcano shader failed to build — volcano will not render." << std::endl;
        g_showVolcano = false;
    } else {
        g_volcano.init();
    }

    // Dedicated T-rex shader. Same fail-soft pattern as the volcano.
    g_trexProgram = buildProgramFromFiles(TREX_VERT_PATH, TREX_FRAG_PATH);
    if (g_trexProgram == 0) {
        std::cerr << "T-rex shader failed to build — T-rex will not render." << std::endl;
        g_showTrex = false;
    } else {
        g_trex.init();
    }

    // Dedicated Brachiosaurus shader.
    g_brachioProgram = buildProgramFromFiles(BRACHIO_VERT_PATH, BRACHIO_FRAG_PATH);
    if (g_brachioProgram == 0) {
        std::cerr << "Brachio shader failed to build — Brachio will not render." << std::endl;
        g_showBrachio = false;
    } else {
        g_brachio.init();
    }
    // (Removed g_meshProgram — Hole16's Mesh class now binds the SAME shader
    // as Hole13/14/15 and the Drone. The Mesh::draw method sets uModel +
    // uNormalMat directly, and feeds the per-mesh r/g/b through a constant
    // vertex attribute at slot 2 so the shader's turf/kerb material
    // classifier picks the right procedural shading. This means every
    // hole gets the same look-and-feel: green-dominant colours get the
    // turf treatment, grey colours get concrete-kerb, etc.)

    Hole13 hole13;  g_h13 = &hole13;
    hole13.init(program);
    Hole14 hole14;  g_h14 = &hole14;
    hole14.init(program);
    // ADD HOLE 15: declare, point g_h15 at, and init the new instance here.
    Hole15 hole15;  g_h15 = &hole15;
    hole15.init(program);

    // Hole 16 is built via Shav's Mesh class. Its constructor does all the
    // geometry generation and water-mesh allocation; no explicit init(program)
    // call is needed because each Mesh inside Hole16 grabs the shader handle
    // at draw time via glGetUniformLocation. We just need to keep a pointer.
    Hole16 hole16;  g_h16 = &hole16;

    // Diya's holes — same Mesh-based pattern. Constructors build geometry,
    // translate() positions the whole hole in world space. No init(program)
    // needed; Mesh::draw picks up uniforms at draw time.
    Hole5 hole5;  g_h5 = &hole5;  hole5.translate(-42.0f, 0.35f, -22.5f);
    Hole6 hole6;  g_h6 = &hole6;  hole6.translate(-14.0f, 0.35f,  -7.5f);
    Hole7 hole7;  g_h7 = &hole7;  hole7.translate( 14.0f, 0.35f,   7.5f);
    Hole8 hole8;  g_h8 = &hole8;  hole8.translate( 42.0f, 0.35f,  22.5f);

    // Holes 10 / 11 / 12 — same Mesh-based pattern as Diya's holes.
    // Positioned at the world origin; the camera reframes onto them via R.
    Hole10 hole10;  g_h10 = &hole10;
    Hole11 hole11;  g_h11 = &hole11;
    Hole12 hole12;  g_h12 = &hole12;

    // Holes 1 / 2 / 3 / 4 — built at local origin; camera reframes via F1-F4.
    Hole1 hole1;  g_h1 = &hole1;
    Hole2 hole2;  g_h2 = &hole2;
    Hole3 hole3;  g_h3 = &hole3;
    Hole4 hole4;  g_h4 = &hole4;

    // Holes 17 / 18 — same Mesh-based pattern. Hole17 (cave) lives at the
    // origin per its local coords (tee x=0, green x=28). Hole18 (S-river)
    // also at origin (tee at x=13, z=0; green at x=12, z=40).
    Hole17 hole17;  g_h17 = &hole17;
    Hole18 hole18;  g_h18 = &hole18;

    // Hole 9 — split halves. Place West (with flag) on -X side and East
    // (tunnel exit) on +X side so they read as a continuous tunnel link.
    Hole9W hole9W;  g_h9W = &hole9W;  hole9W.translate(-15.0f, 0.35f, 0.0f);
    Hole9E hole9E;  g_h9E = &hole9E;  hole9E.translate( 15.0f, 0.35f, 0.0f);

    // Drone shares the same shader program — it uses the same vertex layout
    // (pos / normal / colour) as the holes, so all uniforms line up.
    g_drone.init(program);

    // Dynamic skybox. Day texture / night texture are looked up by relative
    // path; if either BMP is missing, the Skybox falls back to a gradient and
    // just logs a warning rather than crashing.
    g_skybox.dayLengthSecs = 120.0f;   // 2 minutes per full cycle
    g_skybox.init(
        SKYBOX_VERT_PATH,
        SKYBOX_FRAG_PATH,
        "src/assets/textures/sky_day.bmp",
        "src/assets/textures/sky_night.bmp"
    );

    double prevTime = glfwGetTime();
    std::cout
        << "Controls:\n"
        << "  1 / 2 / 3 / 4 / 5 / 6 / 7 / 8 / 9 / 0 / -   select Hole 13/14/15/16/5/6/7/8/10/11/12\n"
        << "  F1 / F2 / F3 / F4                          select Hole 1/2/3/4\n"
        << "  [  / ]                                     select Hole 17 / 18\n"
        << "  ;  / '                                     select Hole 9 West / East\n"
        << "  C              toggle FLY-CAMERA <-> DRONE mode\n"
        << "  R              reframe view on current hole\n"
        << "  B              toggle the volcano landmark on/off\n"
        << "  N              toggle the T-rex landmark on/off\n"
        << "  G              toggle the Brachiosaurus landmark on/off\n"
        << "  Fly-camera (default):\n"
        << "    W/S/A/D      forward / back / strafe left / right\n"
        << "    Space/LShift ascend / descend\n"
        << "    Mouse        look around\n"
        << "  Drone (after pressing C):\n"
        << "    W/S          pitch + translate forward / backward\n"
        << "    A/D          yaw left / right\n"
        << "    Q/E          roll left / right\n"
        << "    Space/LShift ascend / descend\n"
        << "    V            toggle 1st-person <-> 3rd-person\n"
        << "    Z (hold)     zoom in (FoV 60 -> 20)\n"
        << "    Mouse        camera look (1st-person only)\n"
        << "  TAB            toggle wireframe\n"
        << "  M              release / re-capture the mouse cursor\n"
        << "  ESC            quit\n";

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - prevTime);
        prevTime = now;

        if (g_needFrame) { frameCurrent(); g_needFrame = false; }

        // Route input to whichever controller is active. The other one's
        // state stays frozen so toggling back via C resumes where it was.
        if (g_useDrone) {
            g_drone.update(win, dt, g_mouseAccumDx, g_mouseAccumDy);
        } else {
            processMovement(win, dt);
        }
        // Consume the accumulator regardless of mode so a long pause in one
        // mode doesn't dump a giant delta into the other on the next toggle.
        g_mouseAccumDx = 0.0f;
        g_mouseAccumDy = 0.0f;

        // Light sky-blue background (so the holes read as outdoor scenes when
        // the skybox falls back to a gradient — the actual skybox sphere will
        // cover this on every frame the skybox texture/gradient draws).
        glClearColor(0.55f, 0.75f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // View / projection / eye come from whichever controller is driving.
        // The drone's projection() honours its Z-zoom; the fly-camera uses
        // the same fixed 60° FoV as before.
        float aspect = (float)g_winW / (float)g_winH;
        glm::mat4 proj;
        glm::mat4 view;
        glm::vec3 eyePos;
        if (g_useDrone) {
            proj   = g_drone.projection(aspect);
            view   = g_drone.view();
            eyePos = g_drone.eyePos();
        } else {
            proj   = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 500.0f);
            view   = g_cam.view();
            eyePos = g_cam.pos;
        }

        // ── Skybox update + sun/moon read ────────────────────────────────────
        // The skybox rotates a sun + moon around the world X axis. Whichever
        // is currently above the horizon drives the directional light for
        // every hole. The actual sky sphere is drawn at the END of the frame
        // so it sits behind the geometry.
        g_skybox.update(dt);
        SkyLight sun  = g_skybox.getSun();
        SkyLight moon = g_skybox.getMoon();

        // Pick whichever of sun/moon is above the horizon AND has the higher
        // intensity. Both are antipodes on the rotating sphere, so when the
        // sun dips below the horizon (intensity → 0) the moon rises above.
        // Use the active one's direction. Fall back to a steep overhead light
        // if both intensities are zero (only happens for an instant at the
        // horizon).
        glm::vec3 lightDir;
        if (sun.intensity >= moon.intensity * 4.0f && glm::length(sun.position) > 1e-3f) {
            // Daytime — sun dominates (moon is 0.25x scaled in Skybox.cpp,
            // so the 4x factor compensates and picks the sun for the full
            // daylight half of the cycle).
            lightDir = glm::normalize(sun.position);
        } else if (glm::length(moon.position) > 1e-3f) {
            // Night — use the moon's direction so light comes from above.
            lightDir = glm::normalize(moon.position);
        } else {
            lightDir = glm::normalize(glm::vec3(0.45f, 0.80f, 0.35f));
        }
        // Safety net: NEVER let the light direction point below the horizon.
        // Without this clamp, all top-facing surfaces would receive zero
        // diffuse contribution and look pitch-dark whenever both sun and
        // moon are exactly at the horizon. A floor of 0.10 means the
        // steepest grazing angle is ~6° above horizontal — bright enough
        // to read shape, dim enough to feel like dusk.
        if (lightDir.y < 0.10f) {
            lightDir.y = 0.10f;
            lightDir = glm::normalize(lightDir);
        }

        // ── Light colour + ambient driven by time-of-day ─────────────────────
        // Blend factor: 1 = full day (warm white sun, bright fill); 0 = full
        // night (cool blue moon, dim fill). The skybox-Sun's intensity is
        // 0..1 already, so we use it directly.
        float dayBlend = glm::clamp(sun.intensity, 0.0f, 1.0f);

        glm::vec3 dayLight   (1.00f, 0.97f, 0.90f);   // warm noon sun
        glm::vec3 nightLight (0.45f, 0.55f, 0.75f);   // cool moonlight
        glm::vec3 lightColor = glm::mix(nightLight, dayLight, dayBlend);

        // uAmbient is used by the fragment shader as the "ground bounce" tint
        // in the hemisphere ambient model. Slightly brighter during the day,
        // notably dimmer + cooler at night, but NEVER zero — we want every
        // surface to remain readable.
        glm::vec3 dayAmbient   (0.32f, 0.30f, 0.26f);
        glm::vec3 nightAmbient (0.08f, 0.10f, 0.16f);
        glm::vec3 ambient = glm::mix(nightAmbient, dayAmbient, dayBlend);

        // ── Set program-wide uniforms ─────────────────────────────────────────
        // These uniforms used to be set per-hole inside Hole13/14/15::draw().
        // Now they live here so the values stay consistent across all holes
        // and so the Skybox can drive them dynamically. Also uploads
        // view/projection so the hole/mesh draw() methods don't have to.
        glUseProgram(program);
        GLint locView  = glGetUniformLocation(program, "view");
        GLint locProj  = glGetUniformLocation(program, "projection");
        GLint locLC    = glGetUniformLocation(program, "uLightColor");
        GLint locAm    = glGetUniformLocation(program, "uAmbient");
        if (locView != -1) glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        if (locProj != -1) glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));
        if (locLC   != -1) glUniform3fv(locLC, 1, glm::value_ptr(lightColor));
        if (locAm   != -1) glUniform3fv(locAm, 1, glm::value_ptr(ambient));

        switch (g_activeHole) {
            case 13: hole13.draw(view, proj, lightDir, eyePos); break;
            case 14: hole14.draw(view, proj, lightDir, eyePos); break;
            // ADD HOLE 15: add the matching case branch here.
            case 15: hole15.draw(view, proj, lightDir, eyePos); break;

            case 16: {
                // Hole16 uses Shav's Mesh class — now wired up to the SAME
                // shader as the other holes (Mesh::draw sets uModel +
                // uNormalMat and feeds the per-mesh colour via constant
                // attribute slot 2). Just hand it the program and let it
                // render.
                //
                // Hole16 also has an animated water mesh. Hand it the camera
                // + time so the water shader can advance its scrolling.
                g_h16->setCameraData(view, proj, eyePos);
                g_h16->update(dt);
                g_h16->draw(program, g_wireframe);
                break;
            }

            // Diya's Mesh-based holes. Forward the skybox-driven light direction,
            // colour, and eye position so lighting tracks the time-of-day cycle.
            case 5: hole5.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 6: hole6.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 7: hole7.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 8: hole8.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;

            // Holes 10 / 11 / 12 — identical Mesh-based pattern.
            // Program-wide uniforms (view/projection/uLightColor/uAmbient/
            // uLightDir) are already set above; Mesh::draw picks them up.
            case 10: hole10.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 11: hole11.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 12: hole12.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;

            // Holes 1 / 2 / 3 / 4 — same Mesh-based pattern; F1-F4 to select.
            case 1: hole1.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 2: hole2.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 3: hole3.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 4: hole4.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;

            // Hole 17 — cave hole. Plain Mesh-based draw (no water).
            case 17: hole17.draw(program, g_wireframe); break;

            // Hole 9 — same draw signature as Holes 5–8 (forward the
            // skybox-driven light direction, colour, and eye position).
            case 91: hole9W.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;
            case 92: hole9E.draw(program, g_wireframe, lightDir, lightColor, eyePos); break;

            // Hole 18 — S-river hole. Has an animated WaterMesh just like
            // Hole 16, so hand it the camera + dt before drawing.
            case 18: {
                g_h18->setCameraData(view, proj, eyePos);
                g_h18->update(dt);
                g_h18->draw(program, g_wireframe);
                break;
            }

            default: hole13.draw(view, proj, lightDir, eyePos); break;
        }

        // ── Volcano landmark ────────────────────────────────────────────────
        // Drawn AFTER the holes but BEFORE the drone so the drone body still
        // sits in front when you fly past. The volcano has its own shader
        // program (Phong rock + emissive lava with flicker), so we save +
        // restore minimal state: just swap programs and let it do its thing.
        // It's anchored at a fixed offset from the world origin — far enough
        // back that it reads as a background landmark on every hole. Pick
        // your own placement here (or move it per-hole) when you wire it
        // into the final golf-course layout.
        if (g_showVolcano && g_volcanoProgram != 0) {
            glUseProgram(g_volcanoProgram);
            GLint vLocModel    = glGetUniformLocation(g_volcanoProgram, "model");
            GLint vLocView     = glGetUniformLocation(g_volcanoProgram, "view");
            GLint vLocProj     = glGetUniformLocation(g_volcanoProgram, "projection");
            GLint vLocEye      = glGetUniformLocation(g_volcanoProgram, "viewPos");
            GLint vLocLightDir = glGetUniformLocation(g_volcanoProgram, "lightDir");
            GLint vLocLightCol = glGetUniformLocation(g_volcanoProgram, "lightColor");
            GLint vLocAmbient  = glGetUniformLocation(g_volcanoProgram, "ambientColor");
            GLint vLocTime     = glGetUniformLocation(g_volcanoProgram, "time");

            if (vLocView     != -1) glUniformMatrix4fv(vLocView, 1, GL_FALSE, glm::value_ptr(view));
            if (vLocProj     != -1) glUniformMatrix4fv(vLocProj, 1, GL_FALSE, glm::value_ptr(proj));
            if (vLocEye      != -1) glUniform3fv(vLocEye, 1, glm::value_ptr(eyePos));
            // volcano.frag now uses the SAME light-direction convention as
            // fragment.glsl: `lightDir` points FROM surface TOWARD the light,
            // which is exactly what the skybox sun/moon position gives us.
            // No sign flip needed.
            if (vLocLightDir != -1) glUniform3fv(vLocLightDir, 1, glm::value_ptr(lightDir));
            if (vLocLightCol != -1) glUniform3fv(vLocLightCol, 1, glm::value_ptr(lightColor));
            if (vLocAmbient  != -1) glUniform3fv(vLocAmbient, 1, glm::value_ptr(ambient));
            if (vLocTime     != -1) glUniform1f (vLocTime, (float)now);

            // Place the volcano as a background landmark. Behind the holes
            // (+Z direction, since the fly-camera starts at -40 looking +Z)
            // and slightly off to the side. Scale = 2.0 so it reads as a
            // proper mountain (base radius ~12 m, height ~24 m).
            glm::vec3 volcanoPos(35.0f, 0.0f, 70.0f);
            float     volcanoScale = 2.0f;
            g_volcano.draw(vLocModel, volcanoPos, volcanoScale);
        }

        // ── T-rex landmark ─────────────────────────────────────────────────
        // Same render pattern as the volcano: swap to the T-rex program,
        // set its uniforms (the skybox-driven sun direction + day/night
        // colours), then draw. trex.frag uses the SAME lightDir convention
        // as fragment.glsl ("from surface toward light"), so we pass the
        // hole-shader's lightDir unchanged — no sign flip.
        if (g_showTrex && g_trexProgram != 0) {
            glUseProgram(g_trexProgram);
            GLint tLocModel    = glGetUniformLocation(g_trexProgram, "model");
            GLint tLocView     = glGetUniformLocation(g_trexProgram, "view");
            GLint tLocProj     = glGetUniformLocation(g_trexProgram, "projection");
            GLint tLocEye      = glGetUniformLocation(g_trexProgram, "viewPos");
            GLint tLocLightDir = glGetUniformLocation(g_trexProgram, "lightDir");
            GLint tLocLightCol = glGetUniformLocation(g_trexProgram, "lightColor");
            GLint tLocAmbient  = glGetUniformLocation(g_trexProgram, "ambientColor");

            if (tLocView     != -1) glUniformMatrix4fv(tLocView, 1, GL_FALSE, glm::value_ptr(view));
            if (tLocProj     != -1) glUniformMatrix4fv(tLocProj, 1, GL_FALSE, glm::value_ptr(proj));
            if (tLocEye      != -1) glUniform3fv(tLocEye, 1, glm::value_ptr(eyePos));
            if (tLocLightDir != -1) glUniform3fv(tLocLightDir, 1, glm::value_ptr(lightDir));
            if (tLocLightCol != -1) glUniform3fv(tLocLightCol, 1, glm::value_ptr(lightColor));
            if (tLocAmbient  != -1) glUniform3fv(tLocAmbient, 1, glm::value_ptr(ambient));

            // Place the T-rex on the course. Sits in front-left of the
            // volcano so the two read as a "lost-world" backdrop together.
            // Scale = 0.45 turns the 12.5 m raw model into a ~5.6 m
            // decoration (still tall enough to be a clear landmark, not
            // so large it dominates the camera).
            // yaw = -110° rotates it to face slightly inward toward the
            // course, so its roaring head is angled at the player rather
            // than off into the distance.
            glm::vec3 trexPos(15.0f, 0.0f, 55.0f);
            float     trexScale = 0.45f;
            float     trexYaw   = glm::radians(-110.0f);
            g_trex.draw(tLocModel, trexPos, trexScale, trexYaw);
        }

        // ── Brachiosaurus landmark ─────────────────────────────────────────
        // Same render pattern as the T-rex. Placed on the opposite side of
        // the course from the T-rex so the two read as separate creatures
        // rather than facing off. Smaller scale than its raw 13 m frame
        // because it's already very tall; ~0.35 brings it to about 4.6 m
        // tall — roughly the same visual weight as the T-rex.
        if (g_showBrachio && g_brachioProgram != 0) {
            glUseProgram(g_brachioProgram);
            GLint bLocModel    = glGetUniformLocation(g_brachioProgram, "model");
            GLint bLocView     = glGetUniformLocation(g_brachioProgram, "view");
            GLint bLocProj     = glGetUniformLocation(g_brachioProgram, "projection");
            GLint bLocEye      = glGetUniformLocation(g_brachioProgram, "viewPos");
            GLint bLocLightDir = glGetUniformLocation(g_brachioProgram, "lightDir");
            GLint bLocLightCol = glGetUniformLocation(g_brachioProgram, "lightColor");
            GLint bLocAmbient  = glGetUniformLocation(g_brachioProgram, "ambientColor");

            if (bLocView     != -1) glUniformMatrix4fv(bLocView, 1, GL_FALSE, glm::value_ptr(view));
            if (bLocProj     != -1) glUniformMatrix4fv(bLocProj, 1, GL_FALSE, glm::value_ptr(proj));
            if (bLocEye      != -1) glUniform3fv(bLocEye, 1, glm::value_ptr(eyePos));
            if (bLocLightDir != -1) glUniform3fv(bLocLightDir, 1, glm::value_ptr(lightDir));
            if (bLocLightCol != -1) glUniform3fv(bLocLightCol, 1, glm::value_ptr(lightColor));
            if (bLocAmbient  != -1) glUniform3fv(bLocAmbient, 1, glm::value_ptr(ambient));

            // Place it on the opposite side of the volcano from the T-rex,
            // facing roughly across the course toward the holes. Scale 0.35
            // gives a tall but not overwhelming landmark (~4.6 m tall).
            glm::vec3 brachioPos(-25.0f, 0.0f, 60.0f);
            float     brachioScale = 0.35f;
            float     brachioYaw   = glm::radians(80.0f);
            g_brachio.draw(bLocModel, brachioPos, brachioScale, brachioYaw);
        }

        // Draw the drone body LAST so it overlays the hole correctly. It
        // internally no-ops in 1st-person mode, and we also gate it on
        // drone mode being active (you don't want to see the drone hovering
        // in front of you while flying the legacy free camera).
        if (g_useDrone) {
            g_drone.draw(view, proj, lightDir, eyePos);
        }

        // ── Skybox draw (after scene) ────────────────────────────────────────
        // Skybox.cpp handles the depth tricks (disables depth writes,
        // disables culling) internally so we don't need to touch state here.
        // It uses gl_Position.xyww in its vertex shader, so it'll draw at the
        // far plane behind everything when depth-test is on.
        glDepthFunc(GL_LEQUAL);
        g_skybox.draw(view, proj);
        glDepthFunc(GL_LESS);

        std::ostringstream oss;
        oss << "COS344 - Hole " << g_activeHole
            << "  |  ";
        if (g_useDrone) {
            oss << "drone (" << std::fixed << std::setprecision(1)
                << g_drone.position().x << ", " << g_drone.position().y << ", "
                << g_drone.position().z << ")"
                << " [" << (g_drone.thirdPerson() ? "3rd" : "1st") << "-person]";
        } else {
            oss << "cam (" << std::fixed << std::setprecision(1)
                << g_cam.pos.x << ", " << g_cam.pos.y << ", " << g_cam.pos.z << ")"
                << " [fly]";
        }
        glfwSetWindowTitle(win, oss.str().c_str());

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    hole13.cleanup();
    hole14.cleanup();
    // ADD HOLE 15: clean up the new instance here.
    hole15.cleanup();
    // Hole16 cleans itself up in its destructor (Mesh* meshes are deleted
    // there). No explicit cleanup() call needed.
    g_drone.cleanup();
    g_volcano.cleanup();
    if (g_volcanoProgram != 0) glDeleteProgram(g_volcanoProgram);
    g_trex.cleanup();
    if (g_trexProgram != 0) glDeleteProgram(g_trexProgram);
    g_brachio.cleanup();
    if (g_brachioProgram != 0) glDeleteProgram(g_brachioProgram);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}