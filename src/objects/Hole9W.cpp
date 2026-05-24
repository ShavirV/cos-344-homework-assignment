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
#include "Hole9W.h"

Hole9W::Hole9W(float s)
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 borders ───────────────────────────────────────────────────
    Mesh* fairway = new Mesh(Mesh::makeCuboid(28.0f * s, 0.2f * s, 15.0f * s, 0.2f, 0.62f, 0.22f, 1.0f));
    fairway->setup();
    addMesh(fairway);

    // ── Rock obstacle ─────────────────────────────────────────────────────────
    Mesh* rock = new Mesh(Mesh::makeRock(1.0f));
    rock->translate(6.0f, 0.1f, 4.0f);
    rock->setup();
    addMesh(rock);

    Mesh* rock2 = new Mesh(Mesh::makeRock(1.5f));
    rock2->translate(-0.0f, 0.1f, 0.0f);
    rock2->setup();
    addMesh(rock2);

    Mesh* mouth = new Mesh(Mesh::makeTunnel(
    3.5f,   // outer radius
    2.5f,   // inner radius  (1.0 thick wall)
    2.0f,   // depth/length of the tunnel section
    32,
    0.22f, 0.14f, 0.05f, 1.0f));                       
  
    mouth->rotate(90.0f, 0.0f, 0.0f, 1.0f);   // face along X
    mouth->translate(51.0f * s, 33.0f * s, 0.0f);  // translate 
    mouth->setup();
    addMesh(mouth);

    // Mesh* bunker = new Mesh(Mesh::makeCylinderNoTop(
    // 4.0f,                        // radius
    // 2.0f,                       // nearly flat — just enough to not z-fight
    // 32,                          // smoothness
    // 0.76f, 0.70f, 0.50f, 1.0f)); // sand colour

    // bunker->translate(5.0f, 0.11f, 3.0f);
    // bunker->setup();
    // addMesh(bunker);

    Mesh* cup = new Mesh(Mesh::makeCylinder(
    0.5f, 0.3f, 32,
    0.05f, 0.05f, 0.05f, 1.0f));
    cup->translate(-8.0f, 0.0f, 0.0f);  // pick a spot on the fairway
    cup->setup();
    addMesh(cup);

    // ── Flag pole ─────────────────────────────────────────────────────────────
    Mesh* pole = new Mesh(Mesh::makeCylinder(
        0.08f, 6.0f, 8,
        0.9f, 0.9f, 0.9f, 1.0f));
    pole->translate(-8.0f, 3.0f, 0.0f);  // SAME X,Z as cup
    pole->setup();
    addMesh(pole);

    // ── Flag ──────────────────────────────────────────────────────────────────
    Mesh* flag1 = new Mesh(Mesh::makeCuboid(1.5f, 0.8f, 0.3f, 1.0f, 0.1f, 0.1f, 1.0f));
    flag1->translate(-7.25f, 5.5f, 0.0f);  // pole X (-8.0) + half flag width (0.75)
    flag1->setup();
    addMesh(flag1);
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