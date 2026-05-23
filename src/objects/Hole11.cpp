#include "Hole11.h"
Hole11::Hole11(float s)
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway 1 ─────────────────────────────────────────────────────────────
    const float FW = 28.0f * s;
    const float FH =  0.2f * s;
    const float FD = 15.0f * s;

    Mesh* fairway = new Mesh(Mesh::makeCuboid(FW, FH, FD, 1.0f, 0.05f, 0.05f, 1.0f));
    fairway->setup();
    addMesh(fairway);

    // ── Fairway 2 ─────────────────────────────────────────────────────────────
    Mesh* fairway2 = new Mesh(Mesh::makeCuboid(FW, FH, FD, 1.0f, 0.05f, 0.05f, 1.0f));
    fairway2->rotate(90.0f, 0.0f, 1.0f, 0.0f);
    fairway2->translate(14.5f * s, 0.0f, 29.0f * s);
    fairway2->setup();
    addMesh(fairway2);

    // ── Landmass ──────────────────────────────────────────────────────────────
    Mesh* landmass = new Mesh(Mesh::makeCuboid(15.0f * s, 10.0f * s, 10.0f * s, 0.20f, 0.45f, 0.10f, 1.0f));
    landmass->rotate(90.0f, 0.0f, 1.0f, 0.0f);
    landmass->translate(8.0f * s, 1.0f * s, -1.0f * s);
    landmass->setup();
    addMesh(landmass);

    // ── Ramp ──────────────────────────────────────────────────────────────────
    Mesh* hill = new Mesh(Mesh::makeRamp(
        -10.0f * s, 0.0f,        -1.0f * s,
         -1.0f * s, 6.0f * s,   -1.0f * s,
        7.5f * s));
    hill->rotate(90.0f, 0.0f, 1.0f, 0.0f);
    hill->translate(11.0f * s, 0.0f, 13.0f * s);
    hill->setup();
    addMesh(hill);

    // ── Hole cup ──────────────────────────────────────────────────────────────
    Mesh* cup = new Mesh(Mesh::makeCylinder(
        0.5f * s, 0.3f * s, 32,
        0.05f, 0.05f, 0.05f, 1.0f));
    cup->translate(5.0f * s, 0.0f, 2.0f * s);
    cup->setup();
    addMesh(cup);

    // ── Flag pole ─────────────────────────────────────────────────────────────
    Mesh* pole = new Mesh(Mesh::makeCylinder(
        0.08f * s, 6.0f * s, 8,
        0.9f, 0.9f, 0.9f, 1.0f));
    pole->translate(5.0f * s, 3.0f * s, 2.0f * s);
    pole->setup();
    addMesh(pole);

    // ── Flag ──────────────────────────────────────────────────────────────────
    Mesh* flag1 = new Mesh(Mesh::makeCuboid(1.5f * s, 0.8f * s, 0.3f * s, 1.0f, 0.1f, 0.1f, 1.0f));
    flag1->translate(5.75f * s, 5.5f * s, 2.0f * s);
    flag1->setup();
    addMesh(flag1);
}

Hole11::~Hole11()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole11::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole11::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole11::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole11::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}