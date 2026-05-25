#pragma once
#include "../core/Mesh.h"
#include <glm/glm.hpp>

class Hole10
{
public:
    static const int MAX_MESHES = 32;

    Hole10(float scale = 1.0f);
    ~Hole10();

    Hole10(const Hole10&)            = delete;
    Hole10& operator=(const Hole10&) = delete;

    void draw(GLuint shaderProgram, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
    void drawDepth(GLuint depthShader);
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;
    float s;   // scale factor cached for helpers

    void addMesh(Mesh* m);
    void addRock(float x, float y, float z, float scale);
};