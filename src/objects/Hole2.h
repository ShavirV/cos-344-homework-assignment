#pragma once
#include "../core/Mesh.h"

// Hole 2
class Hole2
{
public:
    static const int MAX_MESHES = 8;

    Hole2();
    ~Hole2();

    Hole2(const Hole2&)            = delete;
    Hole2& operator=(const Hole2&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};