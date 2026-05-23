#include "Hole8.h"

// ─────────────────────────────────────────────────────────────────────────────
// Hole 8 — Downhill Drop
//
// The fairway built by buildHoleMeshes represents the TOP (tee) platform.
// Three stepped platforms extend in the +X direction, each 0.8 units lower
// than the previous.  Thin inclined ramps (makeRamp) bridge each step pair
// so the overall surface reads as a smooth descending staircase.
//
// Step geometry (local coords, Y = top-surface of that step):
//   Tee fairway  : centred at X=0,     Y_top=0.2  (28 × 0.2 × 15)
//   Step 1 slab  : centred at X=+18,   Y_top=−0.6 (8 × 0.2 × 12)
//   Step 2 slab  : centred at X=+26,   Y_top=−1.4 (8 × 0.2 × 12)
//   Step 3 slab  : centred at X=+34,   Y_top=−2.2 (8 × 0.2 × 12, landing pad)
//
// Ramp centres sit at the midpoint between adjacent step edges:
//   Ramp A: from (14, 0.2, 0) to (18, -0.6, 0)  — fairway edge to step 1
//   Ramp B: from (22, -0.6, 0) to (26, -1.4, 0) — step 1 to step 2
//   Ramp C: from (30, -1.4, 0) to (34, -2.2, 0) — step 2 to step 3
//
// Each step also gets short concrete border walls on its ±Z sides using
// makeCuboid so the steps look contained.
// ─────────────────────────────────────────────────────────────────────────────

Hole8::Hole8()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Tee fairway + 4 concrete borders ─────────────────────────────────────
    // buildHoleMeshes builds a 28×0.2×15 green slab at the origin.
    Mesh::buildHoleMeshes(0.0f, meshes, meshCount, MAX_MESHES);

    // ── Step colours ──────────────────────────────────────────────────────────
    // Steps use the same bright-red fairway colour as makeHole for consistency.
    const float GR = 1.0f, GG = 0.05f, GB = 0.05f;   // red (makeHole colour)
    const float SH = 0.2f;   // slab height

    // ── Step 1 ────────────────────────────────────────────────────────────────
    const float S1X = 18.0f, S1Y = -0.6f;
    Mesh* step1 = new Mesh(Mesh::makeCuboid(8.0f, SH, 12.0f, GR, GG, GB, 1.0f));
    step1->translate(S1X, S1Y, 0.0f);
    step1->setup();
    addMesh(step1);

    // Step 1 side borders (±Z)
    Mesh* s1bZ = new Mesh(Mesh::makeCuboid(8.0f, 0.4f, 0.5f,
                                            0.85f, 0.85f, 0.85f, 1.0f));
    s1bZ->translate(S1X, S1Y + 0.3f, 6.25f);
    s1bZ->setup();
    addMesh(s1bZ);

    Mesh* s1bZN = new Mesh(Mesh::makeCuboid(8.0f, 0.4f, 0.5f,
                                             0.85f, 0.85f, 0.85f, 1.0f));
    s1bZN->translate(S1X, S1Y + 0.3f, -6.25f);
    s1bZN->setup();
    addMesh(s1bZN);

    // ── Step 2 ────────────────────────────────────────────────────────────────
    const float S2X = 26.0f, S2Y = -1.4f;
    Mesh* step2 = new Mesh(Mesh::makeCuboid(8.0f, SH, 12.0f, GR, GG, GB, 1.0f));
    step2->translate(S2X, S2Y, 0.0f);
    step2->setup();
    addMesh(step2);

    Mesh* s2bZ = new Mesh(Mesh::makeCuboid(8.0f, 0.4f, 0.5f,
                                            0.85f, 0.85f, 0.85f, 1.0f));
    s2bZ->translate(S2X, S2Y + 0.3f, 6.25f);
    s2bZ->setup();
    addMesh(s2bZ);

    Mesh* s2bZN = new Mesh(Mesh::makeCuboid(8.0f, 0.4f, 0.5f,
                                             0.85f, 0.85f, 0.85f, 1.0f));
    s2bZN->translate(S2X, S2Y + 0.3f, -6.25f);
    s2bZN->setup();
    addMesh(s2bZN);

    // ── Step 3 (landing pad / cup) ────────────────────────────────────────────
    const float S3X = 34.0f, S3Y = -2.2f;
    Mesh* step3 = new Mesh(Mesh::makeCuboid(8.0f, SH, 12.0f, GR, GG, GB, 1.0f));
    step3->translate(S3X, S3Y, 0.0f);
    step3->setup();
    addMesh(step3);

    Mesh* s3bZ = new Mesh(Mesh::makeCuboid(8.0f, 0.4f, 0.5f,
                                            0.85f, 0.85f, 0.85f, 1.0f));
    s3bZ->translate(S3X, S3Y + 0.3f, 6.25f);
    s3bZ->setup();
    addMesh(s3bZ);

    Mesh* s3bZN = new Mesh(Mesh::makeCuboid(8.0f, 0.4f, 0.5f,
                                             0.85f, 0.85f, 0.85f, 1.0f));
    s3bZN->translate(S3X, S3Y + 0.3f, -6.25f);
    s3bZN->setup();
    addMesh(s3bZN);

    // ── Ramps connecting each step pair ───────────────────────────────────────
    // makeRamp(x0,y0,z0, x1,y1,z1, width)
    // We use the top-surface Y of each step as the connection Y.
    // Fairway top surface = S_Y + SH/2 = 0.2 + 0.1 = 0.1 at origin (but
    // buildHoleMeshes already centred the slab so its top = 0.1).
    // Step tops: S1 = S1Y + SH/2 = -0.5; S2 = -1.3; S3 = -2.1.
    // We connect the far X edge of the upper to the near X edge of the lower.

    // Ramp A: fairway (+X edge at 14.0) → step 1 (-X edge at 14.0, top −0.5)
    Mesh* rampA = new Mesh(Mesh::makeRamp(
        14.0f,  0.1f, 0.0f,   // fairway far-X edge centre
        14.0f, -0.5f, 0.0f,   // step1 near-X edge centre
        10.0f));               // width (slightly narrower than step)
    addMesh(rampA);

    // Ramp B: step 1 far edge (X=22, top−0.5) → step 2 near edge (X=22, top−1.3)
    Mesh* rampB = new Mesh(Mesh::makeRamp(
        22.0f, -0.5f, 0.0f,
        22.0f, -1.3f, 0.0f,
        10.0f));
    addMesh(rampB);

    // Ramp C: step 2 far edge (X=30, top−1.3) → step 3 near edge (X=30, top−2.1)
    Mesh* rampC = new Mesh(Mesh::makeRamp(
        30.0f, -1.3f, 0.0f,
        30.0f, -2.1f, 0.0f,
        10.0f));
    addMesh(rampC);

    // ── Rock obstacle on landing pad ─────────────────────────────────────────
    Mesh* rock = new Mesh(Mesh::makeRock(1.2f));
    rock->translate(31.0f, S3Y + 0.2f, -3.0f);
    rock->setup();
    addMesh(rock);

    // ── Trees framing the top of the staircase ────────────────────────────────
    // Placed beside the fairway at the point where the first drop begins.
    Mesh* trunk1 = new Mesh(Mesh::makeCylinder(0.4f, 4.5f, 10,
                                                0.36f, 0.25f, 0.13f, 1.0f));
    trunk1->translate(14.5f, 2.25f, -7.5f);
    trunk1->setup();
    addMesh(trunk1);

    Mesh* canopy1 = new Mesh(Mesh::makeCone(2.5f, 5.0f, 14,
                                             0.10f, 0.45f, 0.18f, 1.0f));
    canopy1->translate(14.5f, 6.5f, -7.5f);
    canopy1->setup();
    addMesh(canopy1);

    Mesh* trunk2 = new Mesh(Mesh::makeCylinder(0.4f, 4.5f, 10,
                                                0.36f, 0.25f, 0.13f, 1.0f));
    trunk2->translate(14.5f, 2.25f, 7.5f);
    trunk2->setup();
    addMesh(trunk2);

    Mesh* canopy2 = new Mesh(Mesh::makeCone(2.5f, 5.0f, 14,
                                             0.10f, 0.45f, 0.18f, 1.0f));
    canopy2->translate(14.5f, 6.5f, 7.5f);
    canopy2->setup();
    addMesh(canopy2);
}

Hole8::~Hole8()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole8::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole8::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole8::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole8::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}