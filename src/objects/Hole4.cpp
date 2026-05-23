#include "Hole4.h"
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

// ─── Colours ──────────────────────────────────────────────────────────────────
#define C_GREEN      0.15f, 0.50f, 0.15f, 1.0f   // hill surface
#define C_CREAM      0.93f, 0.90f, 0.80f, 1.0f
#define C_DARKBROWN  0.32f, 0.20f, 0.12f, 1.0f
#define C_BROWN      0.38f, 0.24f, 0.12f, 1.0f
#define C_YELLOW     0.95f, 0.84f, 0.22f, 1.0f
#define C_RED        0.82f, 0.10f, 0.10f, 1.0f
#define C_METAL      0.68f, 0.68f, 0.72f, 1.0f
#define C_WHITE      0.96f, 0.96f, 0.96f, 1.0f
#define C_WOODDARK   0.38f, 0.26f, 0.14f, 1.0f
#define C_DARKGRASS  0.08f, 0.32f, 0.08f, 1.0f
#define C_SAND       0.93f, 0.85f, 0.60f, 1.0f
#define C_FLAGRED    0.90f, 0.10f, 0.10f, 1.0f
#define C_START      0.85f, 0.60f, 0.20f, 1.0f
#define C_BLACK      0.04f, 0.04f, 0.04f, 1.0f
#define C_STEMGREEN  0.20f, 0.55f, 0.20f, 1.0f
#define C_FLWPINK    0.95f, 0.45f, 0.65f, 1.0f
#define C_FLWYELLOW  0.98f, 0.88f, 0.20f, 1.0f

// ─── Windmill scale: ×2 so it fits inside the 15-unit-deep fairway ────────────
static constexpr float S = 2.0f;

// ─── Blade geometry (pivot-local, post-scale) ─────────────────────────────────
static constexpr float SPINE_OFF_X = 0.25f * S;          // 0.50
static constexpr float BLADE_OFF_Y = 0.90f * S;          // 1.80
static constexpr float SPINE_HALF  = (0.90f * S) * 0.5f; // 0.90
// arm bar: from Y=0 up to spine base (BLADE_OFF_Y - SPINE_HALF)
static constexpr float ARM_LEN     = BLADE_OFF_Y - SPINE_HALF; // 0.90
static constexpr float ARM_CY      = ARM_LEN * 0.5f;           // 0.45
static constexpr float SLAT_START  = BLADE_OFF_Y - SPINE_HALF + 0.10f; // first slat Y from pivot
static constexpr float SLAT_STEP   = (SPINE_HALF * 2.0f - 0.10f) / 7.0f; // even spacing across spine

// ─── Hill geometry ────────────────────────────────────────────────────────────
// Fairway footprint: 28 (X) × 15 (Z).  Hill fills it entirely.
// Top (playing surface): 22 × 11  →  gives nice sloped sides all around
// Height: 2.0  →  clearly visible raised mound
static constexpr float HILL_TOP_W  = 22.0f;
static constexpr float HILL_TOP_D  = 11.0f;
static constexpr float HILL_BOT_W  = 28.0f;
static constexpr float HILL_BOT_D  = 15.0f;
static constexpr float HILL_H      =  2.0f;
// Y of the hill top surface relative to local origin (hill is centred at Y=0)
static constexpr float HILL_TOP_Y  = HILL_H * 0.5f;   // 1.0

// ─── Helper: build, translate, setup, pool ───────────────────────────────────
static Mesh* place(Mesh&& src, float x, float y, float z,
                   Mesh** pool, int& cnt, int maxCnt)
{
    Mesh* m = new Mesh(std::move(src));
    m->translate(x, y, z);
    m->setup();
    if (cnt < maxCnt) pool[cnt++] = m;
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Hole4::Hole4()
    : meshCount(0),
      pivotX(0.f), pivotY(0.f), pivotZ(0.f),
      bladeAngle(0.f), lastDrawTime(-1.0)
{
    for (int i = 0; i < MAX_MESHES; ++i) meshes[i] = nullptr;
    for (int i = 0; i < 4; ++i) {
        bladeArm[i] = bladeSpineL[i] = bladeSpineR[i] = nullptr;
        for (int j = 0; j < 8; ++j) bladeSlat[i][j] = nullptr;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 1. HILL  — the raised playing surface
    //    makeHill is centred at its own origin.  We place it at Y=0 (local).
    //    The top surface is at Y = +HILL_TOP_Y = +1.0 above local origin.
    //    Caller will translate the whole group to world space.
    // ─────────────────────────────────────────────────────────────────────────
    {
        Mesh hill = Mesh::makeHill(HILL_TOP_W, HILL_TOP_D,
                                   HILL_BOT_W, HILL_BOT_D,
                                   HILL_H,
                                   C_GREEN);
        // Hill centre at Y=0 local  (top surface at Y=+1.0)
        hill.translate(0.f, 0.f, 0.f);
        hill.setup();
        addMesh(new Mesh(std::move(hill)));
    }

    // Concrete border strips around the base of the hill (off-white kerbs)
    const float BT = 0.5f, BH = 0.5f;
    const float BR = 0.88f, BG = 0.88f, BB = 0.88f;
    // +Z border
    place(Mesh::makeCuboid(HILL_BOT_W, BH, BT, BR, BG, BB, 1.f),
          0.f, -HILL_H * 0.5f + BH * 0.5f, (HILL_BOT_D + BT) * 0.5f,
          meshes, meshCount, MAX_MESHES);
    // -Z border
    place(Mesh::makeCuboid(HILL_BOT_W, BH, BT, BR, BG, BB, 1.f),
          0.f, -HILL_H * 0.5f + BH * 0.5f, -(HILL_BOT_D + BT) * 0.5f,
          meshes, meshCount, MAX_MESHES);
    // +X border
    place(Mesh::makeCuboid(BT, BH, HILL_BOT_D + BT * 2.f, BR, BG, BB, 1.f),
          (HILL_BOT_W + BT) * 0.5f, -HILL_H * 0.5f + BH * 0.5f, 0.f,
          meshes, meshCount, MAX_MESHES);
    // -X border
    place(Mesh::makeCuboid(BT, BH, HILL_BOT_D + BT * 2.f, BR, BG, BB, 1.f),
          -(HILL_BOT_W + BT) * 0.5f, -HILL_H * 0.5f + BH * 0.5f, 0.f,
          meshes, meshCount, MAX_MESHES);

    // ─────────────────────────────────────────────────────────────────────────
    // 2. WINDMILL — centred on the hill top surface
    //    WY = top of hill = HILL_TOP_Y
    //    Windmill X and Z centred at 0 (middle of hill/fairway)
    // ─────────────────────────────────────────────────────────────────────────
    const float WX = 0.f;
    const float WZ = 0.f;
    const float WY = HILL_TOP_Y;   // 1.0 — top of hill

    // Heights measured from WY upward
    const float drumHalfH   = 0.36f * S;           // 0.72
    const float drumTopY    = WY + drumHalfH * 2.f; // WY + 1.44
    const float galleryY    = drumTopY + 0.10f;
    const float shingleY    = galleryY + 0.28f;
    const float coneBaseY   = shingleY + 0.18f;
    const float coneH       = 1.80f * S;             // 3.60
    const float coneCentreY = coneBaseY + coneH * 0.5f;
    const float coneTopY    = coneBaseY + coneH;
    const float hubY        = coneTopY + 0.28f;

    // ── Tunnel base: two cream side slabs (TALLER, reaching all the way up) ────────
    // These slabs now extend from ground level up to the shingle ring
    const float slabW    = 0.20f * S;           // 0.40 width
    const float slabOffX = 0.40f * S;           // 0.80 centre offset from WX
    const float slabD    = 0.65f * S;           // 1.30 depth
    const float slabH    = shingleY - WY;       // TALL: from ground to shingle ring

    // Left slab
    place(Mesh::makeCuboid(slabW, slabH, slabD * 2.f, C_CREAM),
        WX - slabOffX, WY + slabH * 0.5f, WZ, meshes, meshCount, MAX_MESHES);
    // Right slab
    place(Mesh::makeCuboid(slabW, slabH, slabD * 2.f, C_CREAM),
        WX + slabOffX, WY + slabH * 0.5f, WZ, meshes, meshCount, MAX_MESHES);
    // // Top bridge
    // place(Mesh::makeCuboid(slabW, 0.08f * S, slabD * 2.f, C_CREAM),
    //       WX, WY + drumHalfH * 2.f - 0.08f, WZ, meshes, meshCount, MAX_MESHES);
    // Base rim cylinder — radius just outside the slabs
    place(Mesh::makeCylinder(slabD, 0.06f * S, 24, C_DARKBROWN),
          WX, WY + 0.06f, WZ, meshes, meshCount, MAX_MESHES);



    // ── Gallery ring + rail + 12 posts ────────────────────────────────────────
    const float galR = 0.72f * S;   // 1.44
    place(Mesh::makeCylinder(galR,         0.10f * S, 24, C_RED),
          WX, galleryY + 0.05f * S, WZ, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCylinder(galR + 0.04f, 0.05f * S, 24, C_RED),
          WX, galleryY + 0.24f * S, WZ, meshes, meshCount, MAX_MESHES);
    for (int i = 0; i < 12; ++i) {
        float ang = i * 2.f * (float)M_PI / 12.f;
        place(Mesh::makeCuboid(0.04f * S, 0.22f * S, 0.04f * S, C_RED),
              WX + galR * cosf(ang), galleryY + 0.14f * S,
              WZ + galR * sinf(ang), meshes, meshCount, MAX_MESHES);
    }

    // ── Shingle band + cone rim ───────────────────────────────────────────────
    place(Mesh::makeCylinder(0.70f * S, 0.18f * S, 24, C_DARKBROWN),
          WX, shingleY, WZ, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCylinder(0.68f * S, 0.05f * S, 24, C_DARKBROWN),
          WX, shingleY + 0.11f * S, WZ, meshes, meshCount, MAX_MESHES);

    // ── Main cone (yellow hat) ────────────────────────────────────────────────
    place(Mesh::makeCone(0.65f * S, coneH, 24, C_YELLOW),
          WX, coneCentreY, WZ, meshes, meshCount, MAX_MESHES);

    // ── Hub box ───────────────────────────────────────────────────────────────
    place(Mesh::makeCuboid(0.28f * S, 0.28f * S, 0.28f * S, C_DARKBROWN),
          WX, hubY, WZ, meshes, meshCount, MAX_MESHES);

    // ── Axle (rotated 90° around X so it points along +Z) ────────────────────
    const float axleLen    = 0.45f * S;
    const float axleTipZ   = WZ + axleLen * 0.5f + 0.14f * S;
    {
        Mesh axle = Mesh::makeCylinder(0.06f * S, axleLen, 12, C_METAL);
        axle.rotate(90.f, 1.f, 0.f, 0.f);
        axle.translate(WX, hubY, WZ + axleLen * 0.5f);
        axle.setup();
        addMesh(new Mesh(std::move(axle)));
    }
    // Rotor hub disc
    {
        Mesh rh = Mesh::makeCylinder(0.14f * S, 0.12f * S, 12, C_METAL);
        rh.rotate(90.f, 1.f, 0.f, 0.f);
        rh.translate(WX, hubY, axleTipZ);
        rh.setup();
        addMesh(new Mesh(std::move(rh)));
    }

    // Pivot: just in front of the rotor disc
    pivotX = WX;
    pivotY = hubY;
    pivotZ = axleTipZ + 0.07f * S;

    // ── Four rotor arms ───────────────────────────────────────────────────────
    for (int i = 0; i < 4; ++i) buildBlade(i);

    // ── Decorative window cross on +Z face of drum ────────────────────────────
    {
        float wz = WZ + slabD + 0.02f;
        place(Mesh::makeCuboid(0.14f * S, 0.03f * S, 0.03f * S, C_WOODDARK),
              WX + slabOffX, WY + drumHalfH, wz, meshes, meshCount, MAX_MESHES);
        place(Mesh::makeCuboid(0.03f * S, 0.14f * S, 0.03f * S, C_WOODDARK),
              WX + slabOffX, WY + drumHalfH, wz, meshes, meshCount, MAX_MESHES);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. SCENERY on top of the hill
    //    All Z/X positions are LOCAL (relative to hole centre = 0,0).
    //    Keep everything within ±10 X and ±5 Z to stay on the hill top.
    // ─────────────────────────────────────────────────────────────────────────
    const float SY = WY + 0.01f;   // surface Y (top of hill)

    // Trees — behind windmill at -Z edge of hill top
    for (int t = 0; t < 2; ++t) {
        float tx = (t == 0) ? -9.0f : 9.0f;
        float tz = -4.0f;
        place(Mesh::makeCylinder(0.16f, 0.80f, 8, C_BROWN),
              tx, SY + 0.40f, tz, meshes, meshCount, MAX_MESHES);
        place(Mesh::makeCone(0.56f, 1.30f, 10, C_DARKGRASS),
              tx, SY + 0.90f, tz, meshes, meshCount, MAX_MESHES);
    }

    // // Flowers — along right (+X) edge
    // const float FX[4] = { 8.5f,  9.0f,  8.7f,  8.8f};
    // const float FZ[4] = { 3.0f,  1.0f, -1.0f, -3.0f};
    // for (int i = 0; i < 4; ++i) {
    //     float fx = FX[i], fz = FZ[i];
    //     place(Mesh::makeCylinder(0.05f, 0.56f, 8, C_STEMGREEN),
    //           fx, SY + 0.28f, fz, meshes, meshCount, MAX_MESHES);
    //     place(Mesh::makeCylinder(0.12f, 0.08f, 8, C_FLWYELLOW),
    //           fx, SY + 0.60f, fz, meshes, meshCount, MAX_MESHES);
    //     const float PA[4] = {0.f, (float)M_PI*0.5f, (float)M_PI, (float)M_PI*1.5f};
    //     for (int j = 0; j < 4; ++j)
    //         place(Mesh::makeTriangularPrism(0.16f, 0.08f, 0.28f, C_FLWPINK),
    //               fx + 0.20f * cosf(PA[j]), SY + 0.60f,
    //               fz + 0.20f * sinf(PA[j]), meshes, meshCount, MAX_MESHES);
    // }

    // Tee mat — near +Z end of hill top
    place(Mesh::makeCuboid(1.70f, 0.06f, 1.70f, C_START),
          -6.0f, SY + 0.02f, 4.0f, meshes, meshCount, MAX_MESHES);

    // Cup — near -Z end
    place(Mesh::makeCylinder(0.36f, 0.12f, 16, C_BLACK),
          6.0f, SY + 0.02f, -4.5f, meshes, meshCount, MAX_MESHES);

    // Flag pole
    place(Mesh::makeCylinder(0.05f, 1.60f, 8, C_METAL),
          6.0f, SY + 0.86f, -4.5f, meshes, meshCount, MAX_MESHES);

    // Flag banner (rotated to stand vertical)
    Mesh flagBanner = Mesh::makeTriangularPrism(0.56f, 0.06f, 0.36f, C_FLAGRED);
    flagBanner.rotate(90.0f, 0.0f, 0.0f, 1.0f);  // Rotate 90° around Z to make it vertical
    place(std::move(flagBanner), 6.0f, SY + 1.44f, -4.5f, meshes, meshCount, MAX_MESHES);

    // Sand trap
    place(Mesh::makeCylinder(1.10f, 0.08f, 20, C_SAND),
          4.0f, SY + 0.01f, -3.0f, meshes, meshCount, MAX_MESHES);

    // ─────────────────────────────────────────────────────────────────────────
    // 4. Initial blade transforms
    // ─────────────────────────────────────────────────────────────────────────
    updateBladeTransforms();
}

// ─────────────────────────────────────────────────────────────────────────────
// buildBlade
// Each arm: 1 arm bar (hub→spine base) + 2 spines (L & R) + 8 slats spanning
// between the spines.  All geometry at local origin; modelMatrix set each frame.
// ─────────────────────────────────────────────────────────────────────────────
void Hole4::buildBlade(int arm)
{
    // Arm bar: from pivot (Y=0) up to base of spines
    {
        Mesh* ab = new Mesh(Mesh::makeCuboid(0.10f * S, ARM_LEN, 0.10f * S, C_RED));
        ab->setup();
        bladeArm[arm] = ab;
        addMesh(ab);
    }
    // Left spine
    {
        Mesh* sp = new Mesh(Mesh::makeCuboid(0.10f * S, 0.90f * S, 0.10f * S, C_RED));
        sp->setup();
        bladeSpineL[arm] = sp;
        addMesh(sp);
    }
    // Right spine
    {
        Mesh* sp = new Mesh(Mesh::makeCuboid(0.10f * S, 0.90f * S, 0.10f * S, C_RED));
        sp->setup();
        bladeSpineR[arm] = sp;
        addMesh(sp);
    }
    // 8 slats — width spans exactly spine-centre to spine-centre
    for (int j = 0; j < 8; ++j) {
        Mesh* sl = new Mesh(Mesh::makeCuboid(
            SPINE_OFF_X * 2.f, 0.055f * S, 0.04f * S, C_WHITE));
        sl->setup();
        bladeSlat[arm][j] = sl;
        addMesh(sl);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// updateBladeTransforms — called every frame, writes modelMatrix directly
// ─────────────────────────────────────────────────────────────────────────────
void Hole4::updateBladeTransforms()
{
    glm::mat4 toPivot = glm::translate(glm::mat4(1.f),
                                       glm::vec3(pivotX, pivotY, pivotZ));

    for (int arm = 0; arm < 4; ++arm) {
        if (!bladeSpineL[arm]) continue;

        float ang  = bladeAngle + arm * (float)M_PI * 0.5f;
        glm::mat4 spin = glm::rotate(glm::mat4(1.f), ang,
                                     glm::vec3(0.f, 0.f, 1.f));

        // Arm bar: centre at (0, ARM_CY, 0) in local
        if (bladeArm[arm])
            bladeArm[arm]->modelMatrix = toPivot * spin *
                glm::translate(glm::mat4(1.f), glm::vec3(0.f, ARM_CY, 0.f));

        // Left spine
        bladeSpineL[arm]->modelMatrix = toPivot * spin *
            glm::translate(glm::mat4(1.f), glm::vec3(-SPINE_OFF_X, BLADE_OFF_Y, 0.f));

        // Right spine
        bladeSpineR[arm]->modelMatrix = toPivot * spin *
            glm::translate(glm::mat4(1.f), glm::vec3( SPINE_OFF_X, BLADE_OFF_Y, 0.f));

        // Slats evenly spaced along the spine length
        for (int j = 0; j < 8; ++j) {
            if (!bladeSlat[arm][j]) continue;
            float sY = SLAT_START + j * SLAT_STEP;
            bladeSlat[arm][j]->modelMatrix = toPivot * spin *
                glm::translate(glm::mat4(1.f), glm::vec3(0.f, sY, 0.08f * S));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
Hole4::~Hole4()
{
    for (int i = 0; i < meshCount; ++i) delete meshes[i];
}

Mesh* Hole4::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES) meshes[meshCount++] = m;
    return m;
}

void Hole4::draw(GLuint shaderProgram, bool wireframe)
{
    double now = glfwGetTime();
    if (lastDrawTime < 0.0) lastDrawTime = now;
    float dt = (float)(now - lastDrawTime);
    lastDrawTime = now;
    if (dt > 0.1f) dt = 0.1f;

    bladeAngle += BLADE_SPEED * dt;
    if (bladeAngle > 2.f * (float)M_PI)
        bladeAngle -= 2.f * (float)M_PI;

    updateBladeTransforms();

    for (int i = 0; i < meshCount; ++i)
        if (meshes[i]) meshes[i]->draw(shaderProgram, wireframe);
}

void Hole4::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; ++i)
        if (meshes[i]) meshes[i]->translate(x, y, z);
    pivotX += x;  pivotY += y;  pivotZ += z;
}

void Hole4::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; ++i)
        if (meshes[i]) meshes[i]->rotate(angle, ax, ay, az);
}