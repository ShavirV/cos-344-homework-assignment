#include "Hole12.h"
#include <cmath>

Hole12::Hole12()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Hourglass fairway ─────────────────────────────────────────────────────
    // Strategy: divide the 28-unit length into segments along X.
    // Each segment is a cuboid strip whose Z width is determined by a
    // smoothstep-based taper function. Wide at both ends (15), narrow at
    // centre (6).

    const int   SEGS    = 20;         // number of strips
    const float LENGTH  = 28.0f;      // total X length
    const float W_END   = 15.0f;      // Z width at ends
    const float W_MID   =  6.0f;      // Z width at pinch
    const float SEG_LEN = LENGTH / SEGS;
    const float THICK   =  0.2f;      // fairway slab height

    // Width function: smoothly varies from W_END at t=0/1 to W_MID at t=0.5
    // Uses cosine interpolation: w(t) = W_MID + (W_END-W_MID)*0.5*(1+cos(2*pi*t))
    auto widthAt = [&](float t) -> float {
        return W_MID + (W_END - W_MID) * 0.5f * (1.0f + cosf(2.0f * (float)M_PI * t));
    };

    for (int i = 0; i < SEGS; i++)
    {
        float t0 = (float)i       / SEGS;
        float t1 = (float)(i + 1) / SEGS;
        float tM = (t0 + t1) * 0.5f;   // midpoint t for this strip

        float w0 = widthAt(t0);
        float w1 = widthAt(t1);
        float wM = widthAt(tM);         // use midpoint width for the cuboid

        // X centre of this strip
        float xCentre = -LENGTH * 0.5f + (tM * LENGTH);

        Mesh* strip = new Mesh(Mesh::makeCuboid(
            SEG_LEN, THICK, wM,
            1.0f, 0.05f, 0.05f, 1.0f));
        strip->translate(xCentre, 0.0f, 0.0f);
        strip->setup();
        addMesh(strip);

        // ── Border strips on +Z and -Z edges of each fairway segment ─────────
        const float BH = 0.4f;   // border height
        const float BT = 0.4f;   // border thickness in Z

        Mesh* bPos = new Mesh(Mesh::makeCuboid(
            SEG_LEN, BH, BT,
            0.9f, 0.9f, 0.9f, 1.0f));
        bPos->translate(xCentre, 0.0f, wM * 0.5f + BT * 0.5f);
        bPos->setup();
        addMesh(bPos);

        Mesh* bNeg = new Mesh(Mesh::makeCuboid(
            SEG_LEN, BH, BT,
            0.9f, 0.9f, 0.9f, 1.0f));
        bNeg->translate(xCentre, 0.0f, -(wM * 0.5f + BT * 0.5f));
        bNeg->setup();
        addMesh(bNeg);
    }

    // ── End caps (short borders on -X and +X ends) ───────────────────────────
    const float BH  = 0.4f;
    const float BT  = 0.5f;
    const float wL  = widthAt(0.0f);   // width at left end
    const float wR  = widthAt(1.0f);   // width at right end

    Mesh* capL = new Mesh(Mesh::makeCuboid(
        BT, BH, wL + 1.0f,
        0.9f, 0.9f, 0.9f, 1.0f));
    capL->translate(-LENGTH * 0.5f - BT * 0.5f, 0.0f, 0.0f);
    capL->setup();
    addMesh(capL);

    Mesh* capR = new Mesh(Mesh::makeCuboid(
        BT, BH, wR + 1.0f,
        0.9f, 0.9f, 0.9f, 1.0f));
    capR->translate(LENGTH * 0.5f + BT * 0.5f, 0.0f, 0.0f);
    capR->setup();
    addMesh(capR);
}

Hole12::~Hole12()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole12::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole12::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole12::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole12::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}