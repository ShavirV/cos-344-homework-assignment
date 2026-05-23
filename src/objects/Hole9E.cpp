#include "Hole9E.h"

Hole9E::Hole9E()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 borders ───────────────────────────────────────────────────
    Mesh::buildHoleMeshes(0, meshes, meshCount, MAX_MESHES);


    // ── Rock obstacle ─────────────────────────────────────────────────────────
    Mesh* rock = new Mesh(Mesh::makeRock(1.2f));
    rock->translate(-5.0f, 0.1f, -3.0f);
    rock->setup();
    addMesh(rock);

    // ── Tunnel mouth (east) ───────────────────────────────────────────────────
    // Outer ring: brown cylinder, rotated 90° around Z so it faces along X.
//   Mesh* outerMouth = new Mesh(Mesh::makeCylinder(3.5f, 5.5f, 32,
//                                                    0.22f, 0.14f, 0.05f, 1.0f));
//     outerMouth->rotate(90.0f, 0.0f, 0.0f, 1.0f);   // face along X
//     outerMouth->translate(-28.0f, 3.65f, 0.0f);     // west end of the east fairway
//     outerMouth->setup();
//     addMesh(outerMouth);

//     // Inner circle: slightly smaller, very dark cylinder to fake the hollow opening.
//     Mesh* innerMouth = new Mesh(Mesh::makeCylinder(3.0f, 0.2f, 32,
//                                                    0.05f, 0.05f, 0.05f, 1.0f));
//     innerMouth->rotate(90.0f, 0.0f, 0.0f, 1.0f);   // face along X
//     innerMouth->translate(-28.0f, 3.65f, 0.0f);     // same centre as outer
//     innerMouth->setup();
//     addMesh(innerMouth);    // Positioned at the west end of the east fairway, where it exits the volcano.
    // Offset is relative to the fairway origin — translate() moves it with the hole.
  
}

Hole9E::~Hole9E()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole9E::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole9E::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole9E::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole9E::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}