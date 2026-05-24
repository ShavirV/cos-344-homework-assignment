#pragma once
#include "../core/Mesh.h"
#include <glm/glm.hpp>

class Hole2
{
public:
    static const int MAX_MESHES = 200;

    Hole2();
    ~Hole2();

    Hole2(const Hole2&) = delete;
    Hole2& operator=(const Hole2&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);
};