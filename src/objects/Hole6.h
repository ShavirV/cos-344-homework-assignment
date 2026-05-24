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
//
//  KEY FEATURES:
//    • Flat fairway surface (no elevation change — bridge hole)
//    • Stream channel: narrow water strip running along Z at X ≈ 0
//    • Wooden bridge: deck + planks + railing posts (WITH SPHERE CAPS) + handrail
//    • Dinosaur skeleton obstacle: spine, 7-pair rib arches, neck, skull,
//      tail, 4 legs with knees, hip & shoulder bones, joint spheres
//    • Rock formations along both Z borders
//    • Directional sign post near tee
//    • Trees and flowers scattered inside bounds
//    • Cup (black disk) + flag on right pad (+X end)

#pragma once
#include "../core/Mesh.h"
#include <glm/glm.hpp>

class Hole6
{
public:
    // MAX_MESHES breakdown (approx):
    //   Fairway+borders: 6, Stream: 4, Bridge: 6+4(sphere caps)+2+2 = 14
    //   Dino skeleton:   1 spine + 7×3 ribs = 22 + hips 4 + shoulders 4
    //                  + neck 5×2 = 10 + skull 3 + tail 4×2 = 8
    //                  + legs 4×4 = 16  → ~70 dino meshes
    //   SignPost: 3, Rocks: 13, Flag: 3, Trees: 7×3 = 21, Flowers: 11×3 = 33
    //   Total ≈ 170 — set to 200 for headroom.
    static const int MAX_MESHES = 200;

    Hole6();
    ~Hole6();

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

    void buildFairway();
    void buildStream();
    void buildBridge();
    void buildDinoSkeleton();
    void buildSignPost();
    void buildRocks();
    void buildFlag();
    void buildTrees();
    void buildFlowers();

    void addFlagAt(float x, float baseY, float z);
    void addFlowerAt(float x, float baseY, float z, float pr, float pg, float pb);
    void addTree(float x, float baseY, float z, float scale);
};