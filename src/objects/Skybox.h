#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// =============================================================================
// Skybox
//
// A large rotating sphere with:
//   - A scrolling/rotating sky texture (24-bit BMP)
//   - A sun disc mesh on one pole, moon disc on the opposite pole
//   - Two dynamic point lights (sun + moon) that move with the sphere
//
// The sphere rotates around the world X axis (east-west axis), so the sun
// rises in the east, crosses the sky, and sets in the west — just like
// Minecraft's day/night cycle.
//
// Usage:
//   Skybox sky;
//   sky.init("src/shaders/skybox.vert", "src/shaders/skybox.frag",
//            "src/assets/textures/sky.bmp");
//
//   // each frame:
//   sky.update(deltaTime);
//   sky.draw(view, projection);
//
//   // get light data for your main shader:
//   SkyLight sun  = sky.getSun();
//   SkyLight moon = sky.getMoon();
//   // upload sun.position, sun.color, sun.intensity to your shader uniforms
// =============================================================================

struct SkyLight {
    glm::vec3 position;   // world-space position of the light
    glm::vec3 color;      // RGB color
    float     intensity;  // 0..1  (0 at horizon, 1 at zenith)
};

class Skybox {
public:
    Skybox();
    ~Skybox();

    // Call once after GL context is ready.
    // texturePath: 24-bit uncompressed BMP sky panorama.
    // If texturePath is nullptr or fails, falls back to a vertex-color gradient.
    bool init(const char* vertPath,
              const char* fragPath,
              const char* dayTexturePath   = nullptr,
              const char* nightTexturePath = nullptr);

    // dt: seconds since last frame
    void update(float dt);

    // Draw the skybox. Pass your camera view and projection.
    // Depth writing is disabled internally so the sky is always behind everything.
    void draw(const glm::mat4& view, const glm::mat4& projection);

    // Query current sun/moon world positions and light properties.
    SkyLight getSun()  const;
    SkyLight getMoon() const;

    // ── Tweakable ─────────────────────────────────────────────────────────────
    float radius          = 400.0f;  // sphere radius — much larger than the scene
    float dayLengthSecs   = 120.0f;  // seconds for one full day/night cycle
    float sunDiscSize     = 12.0f;   // angular size of sun disc (degrees)
    float moonDiscSize    = 8.0f;

    glm::vec3 sunColor    = glm::vec3(1.00f, 0.95f, 0.80f);
    glm::vec3 moonColor   = glm::vec3(0.55f, 0.65f, 0.85f);
    glm::vec3 skyDayTop   = glm::vec3(0.18f, 0.44f, 0.82f);  // zenith day colour
    glm::vec3 skyDayHoriz = glm::vec3(0.60f, 0.78f, 0.95f);  // horizon day colour
    glm::vec3 skyNightTop = glm::vec3(0.01f, 0.01f, 0.06f);  // zenith night
    glm::vec3 skyNightHoriz=glm::vec3(0.04f, 0.05f, 0.12f);  // horizon night

    int   sphereStacks = 32;
    int   sphereSlices = 48;

private:
    GLuint vao        = 0;
    GLuint vbo        = 0;
    GLuint ebo        = 0;
    GLuint shader     = 0;
    GLuint texDay     = 0;
    GLuint texNight   = 0;
    bool   hasTexDay  = false;
    bool   hasTexNight= false;

    // Sun/moon billboard VAO (two quads drawn as part of the sphere mesh)
    GLuint discVAO = 0;
    GLuint discVBO = 0;

    int   indexCount  = 0;
    float angleRad    = 0.0f;  // current rotation angle around X axis

    // Sun is at +Y pole (top), moon at -Y pole (bottom) in local sphere space.
    // When angleRad = 0, sun is directly overhead.
    glm::vec3 localSunDir()  const { return glm::vec3(0.0f,  1.0f, 0.0f); }
    glm::vec3 localMoonDir() const { return glm::vec3(0.0f, -1.0f, 0.0f); }

    glm::mat4 rotationMatrix() const;

    bool buildShader(const char* vertPath, const char* fragPath);
    bool loadBMP(const char* path, GLuint& outTex);
    void buildSphere();
    void buildDiscs();
    GLint loc(const char* name) const;
};
