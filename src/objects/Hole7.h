// Hole7.h  —  Swamp Hazard Hole (Hole 7)
// Dino Park Phuket — swamp & waterfall zone
//
// FOOTPRINT: 28 (X) × 15 (Z), matching all other holes exactly.
// Pattern: raw pointer array, same as Hole5/Hole6.
//
// LOCAL LAYOUT (X in [-14,+14], Z in [-7.5,+7.5]):
//
//  Tee at -X end, cup/flag at +X end. Flat fairway at Y=0.
//  A swamp pool (murky-green semi-transparent plane) surrounds the fairway
//  on both Z sides, forcing the ball to stay on the narrow central path.
//
//  KEY FEATURES:
//    • Flat narrow fairway strip through the swamp (width ≈ 6 units in Z)
//    • Swamp pool: two murky-green semi-transparent panels flanking fairway
//    • Swamp border mud banks (dark earth-tone raised strips)
//    • Bone obstacle lying across the fairway mid-section
//    • Rock formations clustering around the swamp edges
//    • Trees and flowers at the dry ends
//    • Cup + flag on +X end

#pragma once
#include "../core/Mesh.h"

class Hole7
{
public:
    static const int MAX_MESHES = 130;

    Hole7();
    ~Hole7();

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void draw(GLuint shader, bool wireframe);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);

    void buildFairway();
    void buildSwamp();
    void buildBoneObstacle();
    void buildRocks();
    void buildFlag();
    void buildTrees();
    void buildFlowers();

    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z, float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);
};