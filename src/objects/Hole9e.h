#pragma once
#include "../core/Mesh.h"

// Hole 9 — East exit fairway
//
// All meshes are built at the origin. The caller is responsible for
// positioning the hole via translate() and rotate() before drawing.
// Both methods apply to every mesh in the hole so the whole thing
// moves / rotates as one unit.
//
// MAX_MESHES controls how many mesh objects this hole can hold.
// Increase it as more objects (flags, walls, etc.) are added.
class Hole9E
{
public:
    static const int MAX_MESHES = 8;

    // rotY: optional Y-axis rotation in degrees (0 = length runs along X).
    Hole9E(float rotY = 0.0f);
    ~Hole9E();

    // Non-copyable — meshes own GPU buffers.
    Hole9E(const Hole9E&)            = delete;
    Hole9E& operator=(const Hole9E&) = delete;

    // Draw all meshes belonging to this hole.
    void draw(GLuint shaderProgram, bool wireframe = false);

    // Translate all meshes by (x, y, z).
    void translate(float x, float y, float z);

    // Rotate all meshes around the world origin by angle degrees.
    // Call before translate() to rotate in place about the hole centre.
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];   // owning pointers
    int   meshCount;

    // Add a heap-allocated mesh to the array.
    void addMesh(Mesh* m);
};