#pragma once

// -------------------------------------------------------
// Allowed headers only (per COS344 constraints)
// -------------------------------------------------------
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// -------------------------------------------------------
// WaterMesh — OpenGL 3.3, no external image libraries.
//
// Textures must be 24-bit uncompressed BMP files.
// Free BMP water textures: see README.
// -------------------------------------------------------

struct WaveParams {
    glm::vec2 direction;
    float     steepness;
    float     wavelength;
};

class WaterMesh {
public:
    WaterMesh();
    ~WaterMesh();

    // Call once after GL context is ready.
    // Shader paths: GLSL source files.
    // Texture paths: 24-bit uncompressed BMP files only.
    bool init(const char* vertPath,
              const char* fragPath,
              const char* normalMap0Path,
              const char* normalMap1Path,
              const char* foamMapPath);

    // Call every frame with seconds since last frame.
    void update(float deltaTime);

    // Draw — call AFTER all opaque geometry.
    void draw(const glm::mat4& view,
              const glm::mat4& projection,
              const glm::vec3& cameraPos,
              const glm::vec3& lightDir   = glm::vec3(0.5f, 1.0f, 0.3f),
              const glm::vec3& lightColor = glm::vec3(1.0f, 0.97f, 0.9f));

    // ── Tweakable properties ─────────────────────────────
    glm::mat4 modelMatrix  = glm::mat4(1.0f);

    glm::vec3 colorShallow = glm::vec3(0.35f, 0.65f, 0.60f);
    glm::vec3 colorDeep    = glm::vec3(0.05f, 0.20f, 0.30f);

    float normalTile0   = 4.0f;
    float normalTile1   = 2.0f;
    float scrollSpeed0  = 0.18f;
    float scrollSpeed1  = 0.10f;
    float foamTile      = 6.0f;
    float foamThreshold = 0.08f;

    float fresnelBias   = 0.02f;
    float fresnelScale  = 1.0f;
    float fresnelPower  = 5.0f;

    float specularStrength = 0.9f;
    float shininess        = 256.0f;

    WaveParams waves[4] = {
        { glm::vec2(0.0f,  1.0f), 0.25f, 8.0f },
        { glm::vec2(0.2f,  1.0f), 0.15f, 5.0f },
        { glm::vec2(-0.1f, 1.0f), 0.10f, 3.5f },
        { glm::vec2(0.05f, 0.8f), 0.08f, 2.0f },
    };

    int   gridResolution = 128;
    float planeWidth     = 20.0f;
    float planeLength    = 40.0f;

private:
    GLuint vao       = 0;
    GLuint vbo       = 0;
    GLuint ebo       = 0;
    GLuint shader    = 0;
    GLuint tex0      = 0;
    GLuint tex1      = 0;
    GLuint texFoam   = 0;

    float  accumTime = 0.0f;
    int    indexCount= 0;

    // Uses only stdio.h / stdlib.h — loads 24-bit uncompressed BMP.
    bool loadBMP(const char* path, GLuint& outTex);

    bool buildShader(const char* vertPath, const char* fragPath);
    void buildMesh();
    GLint loc(const char* name) const;
};
