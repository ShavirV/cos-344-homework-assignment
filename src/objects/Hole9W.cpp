#include "Hole9W.h"

Hole9W::Hole9W()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 borders ───────────────────────────────────────────────────
    // buildHoleMeshes allocates all 5 meshes and writes owning pointers into
    // our array. We own them from this point; destructor deletes them.
    Mesh::buildHoleMeshes(0, meshes, meshCount, MAX_MESHES);

    // Apply Y rotation to every mesh now in the array (rotates around origin,
    // which is the hole centre at this point — before any translate).

    // ── Rock obstacle ─────────────────────────────────────────────────────────
    // Added after rotation so its local offset is in the rotated frame.
    Mesh* rock = new Mesh(Mesh::makeRock(1.0f));
    rock->setup();
    addMesh(rock);
}

Hole9W::~Hole9W()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole9W::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole9W::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole9W::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole9W::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}