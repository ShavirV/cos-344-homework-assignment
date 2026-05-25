#pragma once
#include "../core/Mesh.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Hole8
{
public:
    Hole8();
    ~Hole8();

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void draw(GLuint shader, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
    void drawDepth(GLuint depthShader);

private:
    static const int MAX_MESHES = 512;
    Mesh*  meshes[MAX_MESHES];
    int    meshCount;

    void addMesh(Mesh* m);
    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z,
                     float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);

    void buildBorderWalls();
    void buildSteps();
    void buildRisers();
    void buildRocks();
    void buildFlag();
    void buildTrees();
    void buildFlowers();
};