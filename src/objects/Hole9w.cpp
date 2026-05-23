#include "Hole9w.h"

Hole9W::Hole9W(float rotY)
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // Build fairway at the origin — caller translates into world space.
    Mesh* fairway = new Mesh(Mesh::makeHole());
    addMesh(fairway);
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

// Translate every mesh in the hole by (x, y, z).
void Hole9W::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

// Rotate every mesh around the world origin by angle degrees.
// Because all meshes are built at the origin, calling rotate() before
// translate() spins the whole hole about its own centre.
void Hole9W::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}