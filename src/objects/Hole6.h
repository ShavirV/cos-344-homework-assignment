// Hole6.h  —  Bridge & Waterfall Crossing (Hole 6)
// Dino Park Phuket — swamp & waterfall zone
//
// FOOTPRINT: 28 (X) × 15 (Z), matching all other holes exactly.
// Pattern: raw pointer array, same as Hole5.
//
// LOCAL LAYOUT (X in [-14,+14], Z in [-7.5,+7.5]):
//
//  Tee at -X end, cup/flag at +X end. Flat fairway at Y=0 throughout.
//  A wooden bridge spans the central stream channel that cuts across Z.
//  The waterfall wall is visible along the -X border (shared backdrop).
//
//  KEY FEATURES:
//    • Flat fairway surface (no elevation change — bridge hole)
//    • Stream channel: narrow water strip running along Z at X ≈ 0
//    • Wooden bridge: crosses the stream, planks + railing posts + handrail
//    • Rock formations along both Z borders
//    • Directional sign post near tee
//    • Trees and flowers scattered inside bounds
//    • Cup + flag on lower pad (+X end)

#pragma once
#include "../core/Mesh.h"

class Hole6
{
public:
    static const int MAX_MESHES = 140;

    Hole6();
    ~Hole6();

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void draw(GLuint shader, bool wireframe);

private:
    Mesh* meshes[MAX_MESHES];
    int   meshCount;

    void addMesh(Mesh* m);

    void buildFairway();
    void buildStream();
    void buildBridge();
    void buildSignPost();
    void buildRocks();
    void buildFlag();
    void buildTrees();
    void buildFlowers();

    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z, float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);
};