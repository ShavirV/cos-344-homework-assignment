#include "Hole9W.h"

Hole9W::Hole9W()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 borders ───────────────────────────────────────────────────
    Mesh::buildHoleMeshes(0, meshes, meshCount, MAX_MESHES);


    // ── Rock obstacle ─────────────────────────────────────────────────────────
    Mesh* rock = new Mesh(Mesh::makeRock(1.0f));
    rock->translate(6.0f, 0.1f, 4.0f);
    rock->setup();
    addMesh(rock);

Mesh* mouth = new Mesh(Mesh::makeTunnel(
    3.5f,   // outer radius
    2.5f,   // inner radius  (1.0 thick wall)
    2.0f,   // depth/length of the tunnel section
    32,
    0.22f, 0.14f, 0.05f, 1.0f));                       
    mouth->translate(-33.0f, 52.0f, 0.0f);  // move up 5 units
  
mouth->rotate(90.0f, 0.0f, 0.0f, 1.0f);   // face along X
mouth->setup();
addMesh(mouth);
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