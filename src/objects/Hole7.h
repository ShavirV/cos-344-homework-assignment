#pragma once
#include "../core/Mesh.h"

// Hole 7 — Swamp Hazard
//
// Theme: A murky prehistoric swamp surrounds most of the fairway.  The
// fairway is a narrow strip through the centre; the swamp plane fills the
// surrounding area right up to the border walls.  Rock clusters and a pair
// of trees reinforce the boggy atmosphere.
//
// Layout (all built at origin, caller translates):
//   - Fairway slab + 4 concrete borders  (via buildHoleMeshes)
//   - Swamp plane: wide semi-transparent murky-green slab surrounding the
//     fairway (4 panels: left, right, and two short ends)
//   - 3 rock clusters distributed around the swamp edge
//   - 2 trees — one each end of the fairway
//
// MAX_MESHES = 18 is sufficient.
class Hole7
{
public:
    static const int MAX_MESHES = 18;

    Hole7();
    ~Hole7();

    // Non-copyable — meshes own GPU buffers.
    Hole7(const Hole7&)            = delete;
    Hole7& operator=(const Hole7&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};