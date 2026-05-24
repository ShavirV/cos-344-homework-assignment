#pragma once
#include <GL/glew.h>
#include "../core/Mesh.h"

class Hole12
{
public:
    Hole12(float s =1.0f);
    ~Hole12();
    void draw(GLuint shaderProgram, bool wireframe);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    static const int MAX_MESHES = 70;
    Mesh* meshes[MAX_MESHES];
    int   meshCount;
    void  addMesh(Mesh* m);
};