#pragma once
#include "../core/Mesh.h"

class Hole3
{
public:
    static const int MAX_MESHES = 200;
    // Add this alongside the other static factory declarations in Mesh.h:
static Mesh makeRamp2(float x0, float y0, float z0,
                      float x1, float y1, float z1,
                      float width, float thickness = 0.2f);

    Hole3();
    ~Hole3();

    Hole3(const Hole3&) = delete;
    Hole3& operator=(const Hole3&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};