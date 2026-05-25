#pragma once
#include "../core/Mesh.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Hole9E
{
public:
    static const int MAX_MESHES = 256;

    Hole9E(float s = 1.0f);
    ~Hole9E();

    // Non-copyable — meshes own GPU buffers.
    Hole9E(const Hole9E&)            = delete;
    Hole9E& operator=(const Hole9E&) = delete;

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

    void draw(GLuint shader, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
    void drawDepth(GLuint depthShader);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;
    float scale;

    void addMesh(Mesh* m);

    void buildFairway();
    void buildTunnel();
    void buildRocks();
    void buildTrees();
    void buildFlowers();

    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z,
                     float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);
};