#include "Hole7.h"

// ─────────────────────────────────────────────────────────────────────────────
// Hole 7 — Swamp Hazard
//
// The fairway (28 × 0.2 × 15) sits at the origin as usual.  A shallow swamp
// plane surrounds it on all four sides, flush with the fairway top surface.
//
// Swamp panels (murky dark green, alpha ~0.7):
//   Each panel is a thin cuboid (height 0.05) placed just outside the
//   fairway + border region to represent standing swamp water.
//   Panel dimensions chosen so the swamp reads as a continuous body of
//   water with the concrete border kerbs acting as the containment edge.
//
//   Panel layout:
//     Long sides  (±Z, parallel to X):  28 wide × 0.05 tall × 6 deep
//     Short sides (±X, parallel to Z):  6  wide × 0.05 tall × 27 deep
//       (short sides are slightly inset in X so corners don't overlap)
//
// Rocks are placed along the swamp edge to break up the flatness.
// Trees stand at the tee (-X) and cup (+X) ends.
// ─────────────────────────────────────────────────────────────────────────────

Hole7::Hole7()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 concrete borders ─────────────────────────────────────────
    Mesh::buildHoleMeshes(0.0f, meshes, meshCount, MAX_MESHES);

    // ── Swamp plane — 4 panels surrounding the fairway ───────────────────────
    // Murky green: r=0.12, g=0.30, b=0.18 at alpha 0.70
    const float SR = 0.12f, SG = 0.30f, SB = 0.18f, SA = 0.70f;
    const float SH = 0.05f;   // swamp panel height (very thin, flush with surface)

    // +Z panel (beyond waterfall / north edge)
    Mesh* swampZPos = new Mesh(Mesh::makeCuboid(30.0f, SH, 6.0f, SR, SG, SB, SA));
    swampZPos->translate(0.0f, 0.125f, 10.5f);
    swampZPos->setup();
    addMesh(swampZPos);

    // -Z panel (south edge)
    Mesh* swampZNeg = new Mesh(Mesh::makeCuboid(30.0f, SH, 6.0f, SR, SG, SB, SA));
    swampZNeg->translate(0.0f, 0.125f, -10.5f);
    swampZNeg->setup();
    addMesh(swampZNeg);

    // +X panel (cup / exit end)
    Mesh* swampXPos = new Mesh(Mesh::makeCuboid(6.0f, SH, 27.0f, SR, SG, SB, SA));
    swampXPos->translate(17.0f, 0.125f, 0.0f);
    swampXPos->setup();
    addMesh(swampXPos);

    // -X panel (tee end)
    Mesh* swampXNeg = new Mesh(Mesh::makeCuboid(6.0f, SH, 27.0f, SR, SG, SB, SA));
    swampXNeg->translate(-17.0f, 0.125f, 0.0f);
    swampXNeg->setup();
    addMesh(swampXNeg);

    // ── Rock clusters around swamp edge ───────────────────────────────────────
    // Rock A: left side of fairway, mid-way
    Mesh* rockA = new Mesh(Mesh::makeRock(1.3f));
    rockA->translate(-6.0f, 0.2f, -6.5f);
    rockA->setup();
    addMesh(rockA);

    // Rock B: right side, near cup
    Mesh* rockB = new Mesh(Mesh::makeRock(1.0f));
    rockB->translate(7.5f, 0.2f, 5.0f);
    rockB->setup();
    addMesh(rockB);

    // Rock C: just past the tee, partially in swamp — visual hazard marker
    Mesh* rockC = new Mesh(Mesh::makeRock(0.8f));
    rockC->translate(-9.0f, 0.2f, 0.0f);
    rockC->setup();
    addMesh(rockC);

    // ── Trees — tee end and cup end ───────────────────────────────────────────
    // Tee-end tree (-X side, out of play)
    Mesh* trunk1 = new Mesh(Mesh::makeCylinder(0.45f, 5.0f, 10,
                                                0.36f, 0.25f, 0.13f, 1.0f));
    trunk1->translate(-13.0f, 2.5f, 6.5f);
    trunk1->setup();
    addMesh(trunk1);

    Mesh* canopy1 = new Mesh(Mesh::makeCone(2.8f, 5.5f, 14,
                                             0.10f, 0.40f, 0.16f, 1.0f));
    canopy1->translate(-13.0f, 7.0f, 6.5f);
    canopy1->setup();
    addMesh(canopy1);

    // Cup-end tree (+X side)
    Mesh* trunk2 = new Mesh(Mesh::makeCylinder(0.40f, 4.5f, 10,
                                                0.36f, 0.25f, 0.13f, 1.0f));
    trunk2->translate(13.0f, 2.25f, -6.0f);
    trunk2->setup();
    addMesh(trunk2);

    Mesh* canopy2 = new Mesh(Mesh::makeCone(2.5f, 5.0f, 14,
                                             0.10f, 0.40f, 0.16f, 1.0f));
    canopy2->translate(13.0f, 6.5f, -6.0f);
    canopy2->setup();
    addMesh(canopy2);
}

Hole7::~Hole7()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole7::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole7::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole7::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole7::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}