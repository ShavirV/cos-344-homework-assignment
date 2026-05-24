// =============================================================================
// dev.cpp
// Standalone viewer for COS344 mini-golf course holes.
//
// Hosts the team's hole renderers in one viewer with two camera modes and a
// shared dynamic skybox driving the time-of-day lighting.
//
// Holes:
//   1 / 2 / 3   - Ayush's Hole 13 / 14 / 15   (uMVP/uModel/uNormalMat shader)
//   4           - Shav's   Hole 16            (model/view/projection shader)
//
// Each hole's class sets its own uniforms in its draw() call, so the two
// shader programs coexist without stepping on each other — the active hole
// just calls glUseProgram(...) on its own program before drawing.
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
//   1 / 2 / 3 / 4  switch to Hole 13 / 14 / 15 / 16
//   C              toggle FLY-CAMERA <-> DRONE
//   R              re-frame the active controller on the current hole
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

#include "../camera/Drone.h"


// -----------------------------------------------------------------------------
// Shader source paths. Two shader programs are needed because the two halves
// of the project use different shader designs:
//
//   • Ayush's program — src/shaders/vertex.glsl + fragment.glsl
//     Holes 13/14/15 + Drone. Uses uMVP/uModel/uNormalMat + procedural
//     turf/kerb noise + hemisphere ambient.
//
//   • Shav's program — src/shaders/mesh_vertex.glsl + mesh_fragment.glsl
//     Hole 16 (via Mesh.cpp). Uses model/view/projection + a flat 'colour'
//     uniform set per-mesh in Mesh::draw().
//
// They can't share a single program because:
//   - Different uniform names and matrix-construction conventions
//   - Different vertex attribute layout in slot 2 (vec3 colour vs vec2 texCoord)
// So each hole's class binds its own program before drawing.
//
// Paths are relative to the working directory the program is launched from.
// Run from the repo root so "src/shaders/..." resolves correctly.
// -----------------------------------------------------------------------------
static const char* VERTEX_PATH       = "src/shaders/vertex.glsl";
static const char* FRAGMENT_PATH     = "src/shaders/fragment.glsl";
static const char* MESH_VERTEX_PATH  = "src/shaders/mesh_vertex.glsl";
static const char* MESH_FRAGMENT_PATH= "src/shaders/mesh_fragment.glsl";
static const char* SKYBOX_VERT_PATH  = "src/shaders/skybox.vert";
static const char* SKYBOX_FRAG_PATH  = "src/shaders/skybox.frag";

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
// its current sun/moon direction is fed into both shader programs each frame
// as the directional light, so 13/14/15/16 all track the same time of day.
static Hole16* g_h16     = NULL;
static Skybox  g_skybox;
// Second shader program for friend's Mesh-based holes (16). The main program
// (yours) is built later in main() and stored as a local `program`; this
// pointer-free static is just a separate GL handle.
static GLuint  g_meshProgram = 0;

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
    // Friend's shader (Mesh + Hole16). Stored in a global so the render loop
    // can switch to it before drawing Hole16 without a parameter shuffle.
    g_meshProgram = buildProgramFromFiles(MESH_VERTEX_PATH, MESH_FRAGMENT_PATH);

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
        << "  1 / 2 / 3 / 4  select Hole 13 / 14 / 15 / 16\n"
        << "  C              toggle FLY-CAMERA <-> DRONE mode\n"
        << "  R              reframe view on current hole\n"
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
        // one is currently above the horizon (higher intensity) drives the
        // directional light for every hole's shading. The actual sky sphere
        // is drawn at the END of the frame so it sits behind all geometry.
        g_skybox.update(dt);
        SkyLight sun  = g_skybox.getSun();
        SkyLight moon = g_skybox.getMoon();
        SkyLight active = (sun.intensity > moon.intensity) ? sun : moon;

        // Direction TO the light (sun.position is in world space at the sphere
        // radius from origin; the hole shader expects a direction, so just
        // normalise the position vector). Fall back to a steep overhead light
        // if both sun and moon are exactly at the horizon at the same instant.
        glm::vec3 sunDir;
        if (glm::length(active.position) > 1e-3f) {
            sunDir = glm::normalize(active.position);
        } else {
            sunDir = glm::normalize(glm::vec3(0.45f, 0.80f, 0.35f));
        }

        switch (g_activeHole) {
            case 13: hole13.draw(view, proj, sunDir, eyePos); break;
            case 14: hole14.draw(view, proj, sunDir, eyePos); break;
            // ADD HOLE 15: add the matching case branch here.
            case 15: hole15.draw(view, proj, sunDir, eyePos); break;

            case 16: {
                // Friend's Hole16 uses Shav's Mesh class which renders against
                // a different shader program (uniforms: model/view/projection
                // + flat 'colour'). We bind that program, set the per-frame
                // view + projection uniforms once, then call hole16.draw().
                // Inside, each Mesh sets its OWN model + colour uniforms.
                glUseProgram(g_meshProgram);
                GLint vLoc = glGetUniformLocation(g_meshProgram, "view");
                GLint pLoc = glGetUniformLocation(g_meshProgram, "projection");
                if (vLoc != -1) glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(view));
                if (pLoc != -1) glUniformMatrix4fv(pLoc, 1, GL_FALSE, glm::value_ptr(proj));

                // Hole16 has an animated water mesh. Hand it the camera +
                // time so it can render correctly this frame.
                g_h16->setCameraData(view, proj, eyePos);
                g_h16->update(dt);
                g_h16->draw(g_meshProgram, g_wireframe);
                break;
            }

            default: hole13.draw(view, proj, sunDir, eyePos); break;
        }

        // Draw the drone body LAST so it overlays the hole correctly. It
        // internally no-ops in 1st-person mode, and we also gate it on
        // drone mode being active (you don't want to see the drone hovering
        // in front of you while flying the legacy free camera).
        if (g_useDrone) {
            g_drone.draw(view, proj, sunDir, eyePos);
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
    glDeleteProgram(program);
    if (g_meshProgram) glDeleteProgram(g_meshProgram);
    glfwTerminate();
    return 0;
}