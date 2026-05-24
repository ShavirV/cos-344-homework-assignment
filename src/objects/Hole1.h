#pragma once
#include "../core/Mesh.h"

class Hole1
{
public:
    static const int MAX_MESHES = 200;  // Increased for all decorations

    Hole1();
    ~Hole1();

    Hole1(const Hole1&) = delete;
    Hole1& operator=(const Hole1&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};