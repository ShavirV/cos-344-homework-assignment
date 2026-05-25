// Hole9E.h  —  Hole 9 East exit fairway
// Dino Park Phuket
//
// STRICT FOOTPRINT: 28 (X) × 15 (Z), centred at local origin (matches Hole5–8).
//
// Same public interface as Hole7 / Hole8 — draw() takes the program-wide
// light direction, light colour, and camera position so the time-of-day
// skybox can drive lighting consistently across every hole.

#pragma once
#include "../core/Mesh.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Hole9E
{
public:
    static const int MAX_MESHES = 256;

    // s : optional uniform scale applied to the base fairway footprint.
    Hole9E(float s = 1.0f);
    ~Hole9E();

    // Non-copyable — meshes own GPU buffers.
    Hole9E(const Hole9E&)            = delete;
    Hole9E& operator=(const Hole9E&) = delete;

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

    // Draw signature now matches Hole7 / Hole8 so dev.cpp can forward the
    // skybox-driven sun/moon light direction, blended day/night light
    // colour, and camera eye position.
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