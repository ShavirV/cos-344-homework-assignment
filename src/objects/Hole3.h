#pragma once
#include "../core/Mesh.h"

// Hole 3
class Hole3
{
public:
    static const int MAX_MESHES = 8;

    Hole3();
    ~Hole3();

    Hole3(const Hole3&)            = delete;
    Hole3& operator=(const Hole3&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};