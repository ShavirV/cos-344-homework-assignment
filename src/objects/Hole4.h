#pragma once
#include "../core/Mesh.h"
<<<<<<< HEAD
#include <glm/glm.hpp>
=======
>>>>>>> e1d69e93665d0f3fb5b714fcf89405de568fb9a6

// Hole 4 — windmill obstacle on a raised hill-shaped green.
// Build at local origin; caller does: hole4.translate(-48.0f, 0.35f, 15.0f)
// The hill top is 20×9, base 28×15, height 2.5 — matches the reference image
// raised-mound look.  Windmill is ×2 scale so it fits inside the 15-deep fairway.

class Hole4
{
public:
    static const int MAX_MESHES = 512;

    Hole4();
    ~Hole4();

<<<<<<< HEAD
    void draw(GLuint shaderProgram, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
=======
    void draw(GLuint shaderProgram, bool wireframe = false);
>>>>>>> e1d69e93665d0f3fb5b714fcf89405de568fb9a6
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);

private:
    Mesh*  meshes[MAX_MESHES];
    int    meshCount;

    // Animated blade parts
    Mesh*  bladeArm   [4];
    Mesh*  bladeSpineL[4];
    Mesh*  bladeSpineR[4];
    Mesh*  bladeSlat  [4][8];

    float  pivotX, pivotY, pivotZ;
    float  bladeAngle;
    static constexpr float BLADE_SPEED = 1.0f;
    double lastDrawTime;

    Mesh*  addMesh(Mesh* m);
    void   buildBlade(int arm);
    void   updateBladeTransforms();
};