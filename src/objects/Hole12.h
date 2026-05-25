#pragma once
#include <GL/glew.h>
#include "../core/Mesh.h"
#include <glm/glm.hpp>

class Hole12
{
public:
    Hole12(float s = 1.0f);
    ~Hole12();

    void draw(GLuint shaderProgram, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
    void drawDepth(GLuint depthShader);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    static const int MAX_MESHES = 70;
    Mesh* meshes[MAX_MESHES];
    int   meshCount;
    void  addMesh(Mesh* m);
};