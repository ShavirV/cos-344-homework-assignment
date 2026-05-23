#pragma once
#include "../core/Mesh.h"

// Hole 1 — START
class Hole1
{
public:
    static const int MAX_MESHES = 8;

    #define C_GREEN      0.15f, 0.50f, 0.15f, 1.0f
    #define C_DARKGRASS  0.08f, 0.32f, 0.08f, 1.0f

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