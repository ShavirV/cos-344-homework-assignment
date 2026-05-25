#pragma once
#include "../core/Mesh.h"
#include <glm/glm.hpp>

class Hole4
{
public:
    static const int MAX_MESHES = 512;

    Hole4();
    ~Hole4();

    void draw(GLuint shaderProgram, bool wireframe = false,
              const glm::vec3& lightDir   = glm::vec3(0.45f, 0.80f, 0.35f),
              const glm::vec3& lightColor = glm::vec3(1.00f, 0.97f, 0.90f),
              const glm::vec3& camPos     = glm::vec3(0.0f));
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