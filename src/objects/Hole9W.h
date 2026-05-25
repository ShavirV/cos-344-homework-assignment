#pragma once
#include "../core/Mesh.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Hole9W
{
public:
    static const int MAX_MESHES = 256;

    Hole9W(float s = 1.0f);
    ~Hole9W();

    Hole9W(const Hole9W&)            = delete;
    Hole9W& operator=(const Hole9W&) = delete;

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
    void buildFlag();
    void buildTrees();
    void buildFlowers();

    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z,
                     float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);
};