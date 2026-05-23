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
    rock->setup();
    addMesh(rock);
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