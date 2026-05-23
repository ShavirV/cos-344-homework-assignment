#pragma once
#include "../core/Mesh.h"

// Hole 8 — Downhill Drop (staircase)
//
// Theme: After the waterfall & swamp zone the fairway descends via three
// stepped concrete platforms.  The ball accelerates down each step.  This
// is the zone transition hole leading into the cave & tunnel zone.
//
// Layout (all built at origin, caller translates):
//   - Fairway slab + 4 concrete borders  (via buildHoleMeshes)
//     This is the top platform (highest elevation, tee end).
//   - Step 1: cuboid slightly lower than the fairway, centred mid-fairway
//   - Step 2: cuboid lower still
//   - Step 3: lowest cuboid — the landing pad / cup platform
//   - Ramp connectors: thin inclined slabs linking each step pair so the
//     ball can roll smoothly (using makeRamp)
//   - 1 rock obstacle on the lower landing area
//   - 2 trees framing the top of the staircase
//
// Elevation plan (Y values of top surface):
//   Fairway (tee)  : Y = 0.2   (standard height after caller translate)
//   Step 1         : Y = 0.2 − 0.8 = −0.6
//   Step 2         : Y = 0.2 − 1.6 = −1.4
//   Step 3 (cup)   : Y = 0.2 − 2.4 = −2.2
//   All steps are 8 (X) × 0.2 (slab height) × 12 (Z) and offset along +X.
//
// MAX_MESHES = 20
class Hole8
{
public:
    static const int MAX_MESHES = 20;

    Hole8();
    ~Hole8();

    // Non-copyable — meshes own GPU buffers.
    Hole8(const Hole8&)            = delete;
    Hole8& operator=(const Hole8&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};