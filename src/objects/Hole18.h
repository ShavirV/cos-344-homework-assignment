#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../core/Mesh.h"
#include "Water.h"
#include <functional>

// =============================================================================
// Hole18 — S-shaped par-4 river hole
//
// Layout (local XZ space, Y = up):
//
//   Tee  (13, ?, 0)  ← high point, top-right of the S
//
//   Top curve: sweeps left (−X) and down (+Z), fairway descends
//   Waist: stream crosses diagonally NE→SW at Z ≈ 18
//   Bridge: wooden deck spans the stream at the waist
//   Bottom curve: sweeps right (+X) and down (+Z)
//   Lake: sits in the concave pocket of bottom curve (right side)
//   Green: (12, ?, 40) ← low end, bottom-right
//
// The stream feeds the lake — it enters from the waist, exits into the
// lake ellipse on the right.  The lake is a static WaterMesh.
//
// All geometry uses Mesh:: factory methods from Mesh.h.
// =============================================================================

class Hole18 {
public:
    Hole18();
    ~Hole18();

    void draw(GLuint shaderProgram, bool wireframe);
    void update(float deltaTime);
    void setCameraData(const glm::mat4& view,
                       const glm::mat4& proj,
                       const glm::vec3& pos);

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void scale(float sx, float sy, float sz);

private:
    static const int MAX_MESHES = 64;
    Mesh*       meshes[MAX_MESHES];
    int         meshCount;

    WaterMesh*  waterStream = nullptr;
    WaterMesh*  waterLake   = nullptr;
    WaterMesh*  waterLake2   = nullptr;

    // Cached camera data for water draw
    glm::mat4 camView = glm::mat4(1.0f);
    glm::mat4 camProj = glm::mat4(1.0f);
    glm::vec3 camPos  = glm::vec3(0.0f);

    void addMesh(Mesh* m);

    // ── Terrain height functions ──────────────────────────────────────────────
    // The S-shape is built from two arc ribbons joined at the waist.
    // Each has its own height function so the terrain flows naturally.
    //
    // Top arc: tee end is high, waist end is lower.
    // Bottom arc: waist end continues at same level, green end is lowest.
    static float topArcHeight(float arcT, float radialT);
    static float botArcHeight(float arcT, float radialT);
};