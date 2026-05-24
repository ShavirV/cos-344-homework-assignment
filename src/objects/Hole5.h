// Hole5.h
#pragma once
// Hole5.h  —  Swamp & Waterfall Entry Hole (Hole 5)
// Dino Park Phuket — swamp & waterfall zone
//
// FOOTPRINT: 28 (X) × 15 (Z), matching all other holes exactly.
// Pattern: raw pointer array, same as Hole9E / Hole9W.
//
// LOCAL LAYOUT (everything within X in [-14,+14], Z in [-7.5,+7.5]):
//
//  -X ◄──────────────────────────────────────────────────► +X
//
//  [WATERFALL (outside wall)]─[UPPER FAIRWAY]─[RAMP]─[LOWER PAD]
//
//  • UPPER fairway: X in [-14, 0],  surface Y = RISE = 2.5
//  • RAMP:          X in [  0, 5],  slopes smoothly from Y=RISE down to Y=0
//  • LOWER pad:     X in [  5,14],  surface Y = 0  ← CUP + FLAG HERE
//
//  • Waterfall placed outside the -X border wall (behind it)
//  • Stream: thin water strip along +Z inside the upper fairway
//  • All rocks, trees, flowers, bone: placed inside ±13 X, ±6.5 Z

#include "../core/Mesh.h"
#include <glm/glm.hpp>

class Hole5
{
public:
    static const int MAX_MESHES = 120;

    Hole5();
    ~Hole5();

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void draw(GLuint shader, bool wireframe,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);

    void buildFairways();
    void buildWaterfall();
    void buildStream();
    void buildRocks();
    void buildBoneObstacle();
    void buildFlag();
    void buildTrees();
    void buildFlowers();

    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z, float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);
};