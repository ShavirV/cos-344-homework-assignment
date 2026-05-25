#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../objects/Water.h"   // WaterMesh

// =============================================================================
// GlobalWater
// =============================================================================
//
// Manages every course-wide body of water as a single object, matching the
// exact integration pattern used by all hole classes in main.cpp:
//
//   // Construction (once, before render loop)
//   GlobalWater globalWater;
//   globalWater.translate(0.0f, 0.0f, 0.0f);   // optional world offset
//
//   // Each frame, inside render loop:
//   globalWater.update(deltaTime);
//   globalWater.setCameraData(view, projection, cameraPos);
//   globalWater.draw(shader, false);
//
// Water bodies owned (in draw order, back-to-front):
//
//   [0] seaLevel    — 400×150 full-course sheet at Y = SEA_Y (-0.60).
//                     Sits just below the ground slab so it appears in every
//                     gap, hole, and depression across the course.
//
//   [1] volcanoLake — 40×40 square around the volcano base at Y = VLAKE_Y.
//                     The crater surroundings form a shallow hot-spring pool.
//
//   [2] westRiver   — 110×8 strip running east–west through the western
//                     holes (5, 6, 7, 8, 9) at Z ≈ -8, Y = RIVER_Y.
//                     Slightly faster scroll and choppier waves than the
//                     ponds to read as flowing water.
//
//   [3] eastPond    — 40×40 pond near holes 10–11 at X ≈ +90, Z ≈ -35.
//                     Still water, very gentle waves.
//
//   [4] northPond   — 30×30 decorative pond on the northern side of the
//                     course at X ≈ -20, Z ≈ +55.  Calm, ornamental.
//
// All bodies share the same shader and texture paths. Adjust the constants
// at the top of GlobalWater.cpp to reposition any body without touching
// the rest of the code.
//
// Non-copyable (owns WaterMesh pointers).
//
class GlobalWater
{
public:
    GlobalWater();
    ~GlobalWater();

    GlobalWater(const GlobalWater&)            = delete;
    GlobalWater& operator=(const GlobalWater&) = delete;

    // Called once per frame before draw().
    void update(float deltaTime);

    // Store the matrices and camera position for the current frame.
    // Must be called before draw() each frame — mirrors the pattern used by
    // Hole16 and Hole18.
    void setCameraData(const glm::mat4& view,
                       const glm::mat4& proj,
                       const glm::vec3& cameraPos);

    // Draw all water bodies.
    // lightDir and lightColor default to the same warm-sun values used
    // throughout main.cpp, so the call site can omit them:
    //   globalWater.draw(shader, false);
    void draw(GLuint shaderProgram, bool wireframe = false,
              glm::vec3 lightDir   = glm::vec3(0.4f, 1.0f, 0.3f),
              glm::vec3 lightColor = glm::vec3(1.0f, 0.95f, 0.85f));

    // Shift the world-space position of every water body by (x,y,z).
    // Call once after construction to align with the course world offset,
    // exactly as you would call h16.translate(...) or h17.translate(...).
    void translate(float x, float y, float z);

private:
    // ── Water body pointers ───────────────────────────────────────────────────
    WaterMesh* seaLevel;     // [0] full-course underlay
    WaterMesh* volcanoLake;  // [1] crater surroundings
    WaterMesh* westRiver;    // [2] flowing river through western holes
    WaterMesh* eastPond;     // [3] still pond near holes 10-11
    WaterMesh* northPond;    // [4] decorative northern pond

    // Convenience array for uniform iteration (update, draw).
    static const int BODY_COUNT = 5;
    WaterMesh* bodies[BODY_COUNT];

    // ── Per-frame camera state (set by setCameraData) ─────────────────────────
    glm::mat4 camView;
    glm::mat4 camProj;
    glm::vec3 camPos;

    // ── Internal helpers ──────────────────────────────────────────────────────

    // Build and initialise a WaterMesh with the given geometry, wave preset,
    // and world-space centre position.  All shader/texture paths are hardcoded
    // here so call sites stay clean.
    static WaterMesh* makeBody(float width, float length, int res,
                               float centreX, float centreY, float centreZ,
                               float rotYDeg,
                               // wave params: 4 × (dir, amp, wavelength)
                               glm::vec2 d0, float a0, float w0,
                               glm::vec2 d1, float a1, float w1,
                               glm::vec2 d2, float a2, float w2,
                               glm::vec2 d3, float a3, float w3,
                               float scroll0, float scroll1,
                               glm::vec3 colorShallow,
                               glm::vec3 colorDeep,
                               float foamThreshold);
};