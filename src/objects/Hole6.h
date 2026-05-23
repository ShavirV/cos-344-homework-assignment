#pragma once
#include "../core/Mesh.h"

// Hole 6 — Swamp & Waterfall Zone (bridge crossing)
//
// Theme: The stream from Hole 5 runs across this fairway. Players reach
// the green by crossing a short wooden bridge. The waterfall continues
// along the +Z side. A single rock guards the bridge approach.
//
// Layout (all built at origin, caller translates):
//   - Fairway slab + 4 concrete border strips  (via buildHoleMeshes)
//   - Stream channel: blue-green slab crossing perpendicular to fairway
//   - Wooden bridge: flat deck + plank strips + railing posts + handrails
//   - Waterfall continuation: 2-tier concrete ledge on +Z side
//   - 1 rock obstacle near bridge approach
//   - 1 palm tree decorating the far (cup) end
//
// MAX_MESHES = 30 — bridge planks and rails push the count up.
class Hole6
{
public:
    static const int MAX_MESHES = 30;

    Hole6();
    ~Hole6();

    // Non-copyable — meshes own GPU buffers.
    Hole6(const Hole6&)            = delete;
    Hole6& operator=(const Hole6&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};