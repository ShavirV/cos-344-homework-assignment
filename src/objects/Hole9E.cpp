#include <iostream>
#include "Hole9E.h"

Hole9E::Hole9E(float s)
    : meshCount(0)
{

    std::cout <<"Hole9E scale = " << s << std::endl;

    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 borders ───────────────────────────────────────────────────
    const float FW = 28.0f * s;
    const float FH =  0.2f * s;
    const float FD = 15.0f * s;

    Mesh* fairway = new Mesh(Mesh::makeCuboid(FW, FH, FD, 1.0f, 0.05f, 0.05f, 1.0f));
    fairway->setup();
    addMesh(fairway);

    // ── Rock obstacle ─────────────────────────────────────────────────────────
    Mesh* rock = new Mesh(Mesh::makeRock(1.2f * s));
    rock->translate(-5.0f * s, 0.1f * s, -3.0f * s);
    rock->setup();
    addMesh(rock);

    // ── Tunnel mouth ──────────────────────────────────────────────────────────
    Mesh* mouth = new Mesh(Mesh::makeTunnel(
        3.5f * s,
        2.5f * s,
        2.0f * s,
        32,
        0.22f, 0.14f, 0.05f, 1.0f));

mouth->rotate(90.0f, 0.0f, 0.0f, 1.0f);        // rotate SECOND
mouth->translate(-12.5f * s, -2.0f * s, 0.0f);  // translate FIRST

mouth->setup();
addMesh(mouth);

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