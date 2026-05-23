#include "Hole1.h"
#include <cmath>

// ─── Colours ──────────────────────────────────────────────────────────────────
#define C_GREEN      0.15f, 0.50f, 0.15f, 1.0f
#define C_DARKGRASS  0.08f, 0.32f, 0.08f, 1.0f

Hole1::Hole1()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ─── BASE GROUND (28 x 15) ───────────────────────────────────────────────
    const float SY = 0.0f;
    const float groundHeight = 0.2f;
    
    Mesh* ground = new Mesh(Mesh::makeCuboid(28.0f, groundHeight, 15.0f, C_DARKGRASS));
    ground->translate(0.0f, SY + groundHeight * 0.5f, 0.0f);
    ground->setup();
    addMesh(ground);

    // ─── L-SHAPED FAIRWAY (ONLY 2 GREEN BOXES, NO BORDERS) ───────────────────
    const float FH = 0.2f;      // fairway height
    const float FW = 5.0f;      // fairway width
    const float pathY = SY + groundHeight + FH * 0.5f;

    // ─── CHANGED: Vertical section — makeHill rising from Z=5 (low, meets
    // horizontal) to Z=-3 (high, tee end).
    //
    // makeHill(topWidth, topDepth, bottomWidth, bottomDepth, height, ...)
    //   bottom = wide end at Z=5 (joins horizontal section, low)
    //   top    = narrow end at Z=-3 (tee end, elevated)
    //
    // The slope height we want: 1.5 units rise over the 10-unit Z run.
    // makeHill centres vertically, so place its Y centre at pathY + slopeH/2
    // so its bottom face is flush with pathY (same level as horizontal section).
    //
    // Width (X): keep FW=5.0 at the top (narrow), slightly wider at base so
    // it visually flares where it meets the horizontal section.
    const float slopeH = 1.5f;   // gentle rise from bottom to top of slope

    Mesh* verticalFairway = new Mesh(Mesh::makeHill(
        FW,          // topWidth    — same width at tee end
        4.0f,        // topDepth    — Z extent at the top (narrow end, tee side)
        FW + 1.0f,   // bottomWidth — slightly flared where it joins horizontal
        6.0f,        // bottomDepth — Z extent at bottom (wide end, join side)
        slopeH,      // height      — the rise
        C_GREEN
    ));
    // Centre X matches original (-6.0), centre Z matches original (-1.0).
    // Centre Y: bottom of hill must sit at pathY, so centre = pathY + slopeH/2.
    verticalFairway->translate(-5.0f, pathY + slopeH * 0.5f, -1.0f);
    verticalFairway->setup();
    addMesh(verticalFairway);

    // Box 2: Horizontal section — shifted from Z=5 to Z=4 so it butts up against
    // the hill base. Hill centre Z=-1, bottomDepth=6 → bottom face ends at Z=+2.
    // Horizontal depth=5, centre Z=4 → front face starts at Z=1.5, overlapping
    // the hill base edge and closing the gap.
    Mesh* horizontalFairway = new Mesh(Mesh::makeCuboid(18.0f, FH, FW, C_GREEN));
    horizontalFairway->translate(1.0f, pathY, 4.0f);
    horizontalFairway->setup();
    addMesh(horizontalFairway);

    // ─── TEE MAT — centred on the hill top ───────────────────────────────────
    // Hill top centre is at X=-6, Z=-1 (same as the hill translate).
    // Y = pathY + slopeH (top surface of hill) + half mat thickness.
    Mesh* teeMat = new Mesh(Mesh::makeCuboid(1.5f, 0.08f, 1.5f, 1.0f, 0.8f, 0.0f, 1.0f));
    teeMat->translate(-5.0f, pathY + slopeH + 0.04f, -1.0f);
    teeMat->setup();
    addMesh(teeMat);

    // White border around tee mat — follows tee mat
    Mesh* teeBorder = new Mesh(Mesh::makeCuboid(1.6f, 0.02f, 1.6f, 1.0f, 1.0f, 1.0f, 1.0f));
    teeBorder->translate(-5.0f, pathY + slopeH + 0.02f, -1.0f);
    teeBorder->setup();
    addMesh(teeBorder);

    // ─── CUP — raised by 0.3 so it's visible above the horizontal section ────
    Mesh* cup = new Mesh(Mesh::makeCylinder(0.3f, 0.1f, 16, 0.04f, 0.04f, 0.04f, 1.0f));
    cup->translate(8.0f, pathY + 0.32f, 4.0f);
    cup->setup();
    addMesh(cup);

    // ─── FLAG POLE — Y raised to match cup ───────────────────────────────────
    Mesh* flagPole = new Mesh(Mesh::makeCylinder(0.04f, 1.2f, 8, 0.68f, 0.68f, 0.72f, 1.0f));
    flagPole->translate(8.0f, pathY + 0.95f, 4.0f);
    flagPole->setup();
    addMesh(flagPole);
}

Hole1::~Hole1()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole1::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole1::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->draw(shaderProgram, wireframe);
}

void Hole1::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->translate(x, y, z);
}

void Hole1::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->rotate(angle, ax, ay, az);
}