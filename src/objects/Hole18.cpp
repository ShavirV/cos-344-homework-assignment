#include "Hole18.h"
#include <cmath>
#include <functional>

// =============================================================================
// Geometry constants
// =============================================================================

// ── Top arc (tee → waist) ────────────────────────────────────────────────────
// Arc sweeps CCW in XZ from 0° (east, +X) to 180° (west, −X).
// Centre is at the origin so the tee starts at (+R_MID, 0) and the waist
// arrives at (−R_MID, 0), i.e. the arc bows northward (+Z is "into screen"
// so the top of the S bows toward the viewer).
//
// Inner edge at radius 6, outer at 13 → band width ≈ 7 units.
// That gives a top half roughly 26 units wide (outer diameter) by 13 deep.
static const float TOP_INNER_R   =  6.0f;
static const float TOP_OUTER_R   = 13.0f;
static const float TOP_START_DEG =  0.0f;   // tee side (east, +X)
static const float TOP_END_DEG   = 170.0f;  // waist side (west, −X)
static const int   TOP_RING_SEGS = 60;
static const int   TOP_RAD_SEGS  = 8;

// ── Bottom arc (waist → green) ────────────────────────────────────────────────
// The bottom curve is centred on a point offset in +Z so the two arcs meet
// at the waist.  Sweeps from 180° (west) to 360°/0° (east, green side).
// This creates the mirror-image lower belly of the S.
static const float BOT_INNER_R   =  6.0f;
static const float BOT_OUTER_R   = 13.0f;
static const float BOT_START_DEG = 180.0f;  // waist side
static const float BOT_END_DEG   = 350.0f;  // green side (east, +X)
static const int   BOT_RING_SEGS = 60;
static const int   BOT_RAD_SEGS  = 8;

// The bottom arc centre is offset so the two arcs are tangent at the waist.
// Top arc: centre at (0,0,0), waist at (−TOP_INNER_R, 0, 0) inner edge.
// Bot arc: we want its 180° point to also be at roughly (−BOT_OUTER_R, 0, BOT_CZ).
// Choose BOT_CZ = 2*(BOT_INNER_R) so the two rings share the waist band.
static const float BOT_CX        = -19.0f;
static const float BOT_CZ        = 0.0f;   // Z offset of bottom arc centre

// ── Terrain heights ───────────────────────────────────────────────────────────
// Top arc: tee (arcT=0) is +2.5 high, waist (arcT=1) is 0.
// Bottom arc: waist (arcT=0) is 0, green (arcT=1) is −0.8 (slight depression).
// Both have a gentle sinusoidal roll plus a radial crown.
static const float TOP_TEE_Y     =  2.5f;   // height at tee (arcT=0)
static const float TOP_ROLL_AMP  =  0.4f;   // sinusoidal roll amplitude
static const float TOP_ROLL_FREQ =  2.0f;   // cycles over the top arc
static const float BOT_ROLL_AMP  =  0.35f;
static const float BOT_ROLL_FREQ =  1.5f;
static const float BOT_GREEN_DIP = -0.8f;   // green end Y offset
static const float CROWN_AMP     =  0.20f;  // cross-slope crown

// ── Border pavers ─────────────────────────────────────────────────────────────
static const float BORDER_H      =  0.30f;
static const float PAVER_W       =  0.40f;

// ── Tee ───────────────────────────────────────────────────────────────────────
// tee is at the 0° point of the top arc, mid-radius = (6+13)/2 = 9.5
// in LOCAL arc space that's at (9.5, topArcHeight(0,0.5), 0).
static const float TEE_R         =  2.0f;
static const float TEE_X         =  9.5f;
static const float TEE_Z         =  0.0f;

// ── Tee stairs ───────────────────────────────────────────────────────────────
static const int   TEE_STAIR_COUNT  = 5;
static const float TEE_STAIR_WIDTH  = 1.4f;
static const float TEE_STAIR_DEPTH  = 3.0f;
static const float TEE_STAIR_RISE   = TOP_TEE_Y / TEE_STAIR_COUNT;

// Start far enough back so last step lands at tee X
static const float TEE_STAIR_START_X =
    TEE_X - (TEE_STAIR_COUNT * TEE_STAIR_WIDTH);

// ── Green ─────────────────────────────────────────────────────────────────────
// Green is at the 360° (=0°) point of the bottom arc, mid-radius = 9.5
// in world space that's (BOT_CX + 9.5, botArcHeight(1,0.5), BOT_CZ).
static const float GREEN_R       =  3.6f;
static const float GREEN_X       =  -19.0f;   // relative to BOT_CX
static const float GREEN_Z_REL   =  0.0f;   // relative to BOT_CZ (it's at 0° of bot arc)

static const float FLAG_POLE_H = 2.2f;
static const float FLAG_POLE_R = 0.05f;

static const float FLAG_W      = 0.7f;
static const float FLAG_H      = 0.4f;
static const float FLAG_D      = 0.02f;

static const float HOLE_CUP_R  = 0.12f;
static const float HOLE_CUP_H  = 0.08f;
// ── Stream ────────────────────────────────────────────────────────────────────
// Flows diagonally across the waist of the S, roughly NE to SW.
// Modelled as a single WaterMesh plane at the waist.
static const float STREAM_Y      = -0.12f;
static const float STREAM_W      =  5.2f;
static const float STREAM_L      = 12.0f;
static const float STREAM_CX     = -5.0f;   // world X of stream centre
static const float STREAM_CZ     = -0.5f;   // world Z (midpoint of waist)
static const float STREAM_ROT    = 275.0f;   // degrees CCW from +X

// ── Lake ─────────────────────────────────────────────────────────────────────
// Sits in the concave pocket of the bottom curve (the right/east side).
// Modelled as a WaterMesh plane.
static const float LAKE_Y        = -0.18f;
static const float LAKE_W        =  15.0f;
static const float LAKE_L        =  15.0f;
static const float LAKE_CX       =  -18.0f;   // world X (right side of bot arc)
static const float LAKE_CZ       = 0.0f;   // world Z (inside bot arc pocket)
static const float LAKE_RETAIN_R    = LAKE_W * 0.5f;   // 7.5 — paver inner face
static const int   LAKE_RETAIN_SEGS = 48;
static const float LAKE1_CX         = LAKE_CX;          // -18.0
static const float LAKE1_CZ         = LAKE_CZ;           //   0.0
static const float LAKE2_CX         = LAKE_CX + 17.0f;  //  -1.0
static const float LAKE2_CZ         = LAKE_CZ;           //   0.0

// ── Bridge ────────────────────────────────────────────────────────────────────
// Wooden deck spanning the stream.  Positioned at stream centre, rotated to
// align perpendicular to stream direction.
static const float BRIDGE_L      =  7.0f;
static const float BRIDGE_W      =  3.8f;
static const float BRIDGE_Y      =  0.10f;
static const float BRIDGE_CX     = STREAM_CX + 4.0f;
static const float BRIDGE_CZ     = STREAM_CZ + 10.0f;
static const float BRIDGE_ROT    = STREAM_ROT + 0.0f; // perpendicular to stream

// ── Hill bumps ────────────────────────────────────────────────────────────────
struct H18Hill { float x; float z; float arcT; float radT; float h; bool topArc; };
static const H18Hill HILLS[] = {
    {  6.0f,  -4.5f, 0.15f, 0.5f, 0.80f, true  },  // top arc, near tee
    {  0.0f,  -8.0f, 0.35f, 0.4f, 0.60f, true  },  // top arc, mid
    { -7.0f,  -5.0f, 0.60f, 0.6f, 0.70f, true  },  // top arc, near waist
    { -3.0f,  24.0f, 0.30f, 0.5f, 0.55f, false },  // bot arc
    {  4.0f,  30.0f, 0.60f, 0.4f, 0.50f, false },  // bot arc, near green
};

// ── Rock obstacles ────────────────────────────────────────────────────────────
struct H18Rock { float x; float z; float arcT; float radT; float scale; bool topArc; };
static const H18Rock ROCKS[] = {
    { -8.0f,  -3.0f, 0.50f, 0.3f, 0.9f, true  },
    { -5.0f,  -7.5f, 0.70f, 0.7f, 1.1f, true  },
};

// =============================================================================
// Height functions
// =============================================================================

float Hole18::topArcHeight(float arcT, float radialT)
{
    // Smoothly descend from TEE_Y at arcT=0 to 0 at arcT=1
    float base  = TOP_TEE_Y * (1.0f - arcT);
    // Rolling hills on top of the descent
    float roll  = TOP_ROLL_AMP * sinf(arcT * TOP_ROLL_FREQ * 2.0f * (float)M_PI);
    // Crown across the band
    float crown = CROWN_AMP * (1.0f - fabsf(radialT * 2.0f - 1.0f));
    return base + roll + crown;
}

float Hole18::botArcHeight(float arcT, float radialT)
{
    // Continue from 0 at waist (arcT=0) to BOT_GREEN_DIP at green (arcT=1)
    float base  = BOT_GREEN_DIP * arcT;
    float roll  = BOT_ROLL_AMP * sinf(arcT * BOT_ROLL_FREQ * 2.0f * (float)M_PI + 1.3f);
    float crown = CROWN_AMP * (1.0f - fabsf(radialT * 2.0f - 1.0f));
    return base + roll + crown;
}

// =============================================================================
// Constructor
// =============================================================================
Hole18::Hole18() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++) meshes[i] = nullptr;

    // ── [0] Top arc fairway ───────────────────────────────────────────────────
    // Standard arc centred at (0,0,0).
    {
        Mesh* top = new Mesh(Mesh::makeArcFairway(
            TOP_INNER_R, TOP_OUTER_R,
            TOP_START_DEG, TOP_END_DEG,
            TOP_RING_SEGS, TOP_RAD_SEGS,
            topArcHeight,
            0.15f, 0.55f, 0.12f, 1.0f,  // fairway green
            4.0f, 2.0f));
        addMesh(top);
    }

    // ── [1] Top arc ground infill (flat, fills under the hilly surface) ───────
    {
        Mesh* topBase = new Mesh(Mesh::makeArcFairway(
            TOP_INNER_R, TOP_OUTER_R,
            TOP_START_DEG, TOP_END_DEG,
            TOP_RING_SEGS, TOP_RAD_SEGS,
            [](float, float){ return 0.0f; },
            0.04f, 0.79f, 0.16f, 1.0f,
            4.0f, 2.0f));
        addMesh(topBase);
    }

    // ── [2] Top arc outer kerb ────────────────────────────────────────────────
    {
        Mesh* ko = new Mesh(Mesh::makeArcBorder(
            TOP_OUTER_R, PAVER_W, BORDER_H,
            TOP_START_DEG, TOP_END_DEG,
            TOP_RING_SEGS, +1, topArcHeight,
            0.76f, 0.74f, 0.70f, 1.0f));
        addMesh(ko);
    }

    // ── [3] Top arc inner kerb ────────────────────────────────────────────────
    {
        Mesh* ki = new Mesh(Mesh::makeArcBorder(
            TOP_INNER_R, PAVER_W, BORDER_H,
            TOP_START_DEG, TOP_END_DEG,
            TOP_RING_SEGS, -1, topArcHeight,
            0.76f, 0.74f, 0.70f, 1.0f));
        addMesh(ki);
    }

    // ── [4] Bottom arc fairway ────────────────────────────────────────────────
    // Bottom arc is centred at (BOT_CX, 0, BOT_CZ) so we build it at the
    // origin and translate, because makeArcFairway centres at origin.
    {
        Mesh* bot = new Mesh(Mesh::makeArcFairway(
            BOT_INNER_R, BOT_OUTER_R,
            BOT_START_DEG, BOT_END_DEG,
            BOT_RING_SEGS, BOT_RAD_SEGS,
            botArcHeight,
            0.15f, 0.55f, 0.12f, 1.0f,
            4.0f, 2.0f));
        bot->translate(BOT_CX, 0.0f, BOT_CZ);
        bot->setup();
        addMesh(bot);
    }

    // ── [5] Bottom arc ground infill ──────────────────────────────────────────
    {
        Mesh* botBase = new Mesh(Mesh::makeArcFairway(
            BOT_INNER_R, BOT_OUTER_R,
            BOT_START_DEG, BOT_END_DEG,
            BOT_RING_SEGS, BOT_RAD_SEGS,
            [](float, float){ return 0.0f; },
            0.04f, 0.79f, 0.16f, 1.0f,
            4.0f, 2.0f));
        botBase->translate(BOT_CX, -0.0f, BOT_CZ);
        botBase->setup();
        addMesh(botBase);
    }

    // ── [6] Bottom arc outer kerb ─────────────────────────────────────────────
    {
        Mesh* bko = new Mesh(Mesh::makeArcBorder(
            BOT_OUTER_R, PAVER_W, BORDER_H,
            BOT_START_DEG, BOT_END_DEG,
            BOT_RING_SEGS, +1, botArcHeight,
            0.76f, 0.74f, 0.70f, 1.0f));
        bko->translate(BOT_CX, 0.0f, BOT_CZ);
        bko->setup();
        addMesh(bko);
    }

    // ── [7] Bottom arc inner kerb ─────────────────────────────────────────────
    {
        Mesh* bki = new Mesh(Mesh::makeArcBorder(
            BOT_INNER_R, PAVER_W, BORDER_H,
            BOT_START_DEG, BOT_END_DEG,
            BOT_RING_SEGS, -1, botArcHeight,
            0.76f, 0.74f, 0.70f, 1.0f));
        bki->translate(BOT_CX, 0.0f, BOT_CZ);
        bki->setup();
        addMesh(bki);
    }

    // ── [8] Large ground disc (base plane, fills all gaps) ────────────────────
    {
        Mesh* gnd = new Mesh(Mesh::makeCylinder(
            TOP_OUTER_R + 4.0f, 0.15f, 64,
            0.22f, 0.16f, 0.09f, 1.0f));
        gnd->translate(0.0f, -1.10f, BOT_CZ * 0.5f);
        gnd->setup();
        addMesh(gnd);
    }

    // ── [9] Tee marker (yellow disc) ─────────────────────────────────────────
    {
        float teeY = topArcHeight(0.0f, 0.5f) + 0.06f;
        Mesh* tee = new Mesh(Mesh::makeCylinder(
            TEE_R, 0.10f, 20,
            0.85f, 0.82f, 0.18f, 1.0f));
        tee->translate(TEE_X, teeY+0.13f, TEE_Z+2);
        tee->setup();
        addMesh(tee);
    }

    // ── [10] Green surface + kerb ring ───────────────────a────────────────────
    {
        // Green is at 360° of bottom arc, mid-radius = (BOT_INNER_R+BOT_OUTER_R)*0.5
        float gx   = BOT_CX + (BOT_INNER_R + BOT_OUTER_R) * 0.5f + GREEN_X;
        float gz   = BOT_CZ + GREEN_Z_REL;
        float gY   = botArcHeight(1.0f, 0.5f) + 1.0f;

        Mesh* green = new Mesh(Mesh::makeCylinder(
            GREEN_R, 0.14f, 24,
            0.08f, 0.62f, 0.10f, 1.0f));
        green->translate(gx, gY, gz);
        green->setup();
        addMesh(green);

        Mesh* gKerb = new Mesh(Mesh::makeCylinder(
            GREEN_R + 0.45f, 0.22f, 24,
            0.76f, 0.72f, 0.66f, 1.0f));
        gKerb->translate(gx, gY - 0.04f, gz);
        gKerb->setup();
        addMesh(gKerb);
    }

        // ── [11] Tee stairs ──────────────────────────────────────────────────────────
{
    for (int i = 0; i < TEE_STAIR_COUNT; i++)
    {
        float xc = TEE_X + (i + 0.5f) * TEE_STAIR_WIDTH;
        float yc = TOP_TEE_Y - (i + 0.5f) * TEE_STAIR_RISE;

        Mesh* step = new Mesh(Mesh::makeCuboid(
            TEE_STAIR_WIDTH,
            TEE_STAIR_RISE,
            TEE_STAIR_DEPTH,
            0.76f, 0.74f, 0.70f, 1.0f));

        step->translate(xc-3, yc, TEE_Z-1.6);
        step->setup();
        addMesh(step);
    }
    {
        Mesh* stairWall = new Mesh(Mesh::makeCuboid(
             BOT_INNER_R+1.9,
             2*TOP_TEE_Y-2,
             0.10f,
             0.76f, 0.74f, 0.70f, 1.0f));

        stairWall->translate(TEE_X, TEE_Z+1, TEE_Z);
        stairWall->setup();
        addMesh(stairWall);
    }

    // ── [24] Flag pole ────────────────────────────────────────────────────────
    {
        // Match exact green centre from [10]
        float gx = BOT_CX + (BOT_INNER_R + BOT_OUTER_R) * 0.5f + GREEN_X;
        float gz = BOT_CZ + GREEN_Z_REL;
        float gY = botArcHeight(1.0f, 0.5f) + 1.0f;

        Mesh* pole = new Mesh(Mesh::makeCylinder(
            FLAG_POLE_R, FLAG_POLE_H, 8,
            0.85f, 0.85f, 0.85f, 1.0f));

        // Cylinder origin is centered vertically
        pole->translate(
            gx,
            gY + FLAG_POLE_H * 0.5f,
            gz
        );

        pole->setup();
        addMesh(pole);
    }

    // ── [25] Flag mesh ────────────────────────────────────────────────────────
    {
        float gx = BOT_CX + (BOT_INNER_R + BOT_OUTER_R) * 0.5f + GREEN_X;
        float gz = BOT_CZ + GREEN_Z_REL;
        float gY = botArcHeight(1.0f, 0.5f) + 1.0f;

        Mesh* flag = new Mesh(Mesh::makeCuboid(
            FLAG_W, FLAG_H, FLAG_D,
            0.85f, 0.10f, 0.08f, 1.0f));

        // Offset from pole so it hangs outward
        flag->translate(
            gx + FLAG_W * 0.5f,
            gY + FLAG_POLE_H * 0.82f,
            gz
        );

        flag->setup();
        addMesh(flag);
    }

    // ── [26] Hole cup ─────────────────────────────────────────────────────────
    {
        float gx = BOT_CX + (BOT_INNER_R + BOT_OUTER_R) * 0.5f + GREEN_X;
        float gz = BOT_CZ + GREEN_Z_REL;
        float gY = botArcHeight(1.0f, 0.5f) + 1.0f;

        Mesh* cup = new Mesh(Mesh::makeCylinder(
            HOLE_CUP_R, HOLE_CUP_H, 16,
            0.05f, 0.05f, 0.05f, 1.0f));

        // Slightly above green to avoid z-fighting
        cup->translate(
            gx,
            gY + 0.01f,
            gz
        );

        cup->setup();
        addMesh(cup);
    }

     // ── [16] Lake 1 retaining wall ────────────────────────────────────────────
    // A full 360° arc-border ring sitting just outside the lake edge.
    // makeArcBorder with a flat heightFn produces a uniform-height ring of
    // 3-D paver slabs — the same quality as the fairway kerbs.
    // side=+1 means pavers extrude OUTWARD (away from the lake centre),
    // so the inner face of each paver lines up with the lake edge.
    {
        Mesh* retain1 = new Mesh(Mesh::makeArcBorder(
            LAKE_RETAIN_R - PAVER_W, // edge radius (inside face of paver)
            PAVER_W, BORDER_H + 0.15f,   // slightly taller than fairway kerbs
            40.0f, 164.0f,
            LAKE_RETAIN_SEGS, +1,
            [](float, float){ return 0.0f; },  // flat — lake surface is level
            0.76f, 0.74f, 0.70f, 1.0f));       // warm sandstone colour
        retain1->translate(LAKE1_CX, LAKE_Y + 0.05f, LAKE1_CZ);
        retain1->setup();
        addMesh(retain1);
    }


    // ── [18] Lake 2 retaining wall ────────────────────────────────────────────
    {
        Mesh* retain2 = new Mesh(Mesh::makeArcBorder(
            LAKE_RETAIN_R - PAVER_W,
            PAVER_W, BORDER_H + 0.15f,
            221.0f, 360.0f,
            LAKE_RETAIN_SEGS, +1,
            [](float, float){ return 0.0f; },
           0.76f, 0.74f, 0.70f, 1.0f));
        retain2->translate(LAKE2_CX, LAKE_Y + 0.05f, LAKE2_CZ);
        retain2->setup();
        addMesh(retain2);
    }


    //── [20] Lake 1 bank disc ─────────────────────────────────────────────────
    //Low cylinder giving the appearance of a bank or levee around the water.
    //Sits just outside the retaining wall.
    {
        Mesh* bank1 = new Mesh(Mesh::makeCylinder(
            LAKE_RETAIN_R + 1.2f, 0.25f, 40,
            0.30f, 0.22f, 0.12f, 1.0f));
        bank1->translate(LAKE1_CX, LAKE_Y - 0.02f, LAKE1_CZ);
        bank1->setup();
        addMesh(bank1);
    }

    // ── [11] Bridge deck ─────────────────────────────────────────────────────
    // Wooden bridge spanning the stream. The deck sits just above stream Y.
    {
        Mesh* deck = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.18f, BRIDGE_W,
            0.52f, 0.33f, 0.14f, 1.0f));
        deck->rotate(BRIDGE_ROT, 0.0f, 1.0f, 0.0f);
        deck->translate(BRIDGE_CX, BRIDGE_Y, BRIDGE_CZ);
        deck->setup();
        addMesh(deck);

        // Left rail
        Mesh* railL = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.40f, 0.12f,
            0.42f, 0.26f, 0.10f, 1.0f));
        railL->rotate(BRIDGE_ROT, 0.0f, 1.0f, 0.0f);
        railL->translate(BRIDGE_CX, BRIDGE_Y + 0.29f,
                         BRIDGE_CZ - cosf(glm::radians(BRIDGE_ROT)) * (BRIDGE_W * 0.5f - 0.1f));
        railL->setup();
        addMesh(railL);

        // Right rail
        Mesh* railR = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.40f, 0.12f,
            0.42f, 0.26f, 0.10f, 1.0f));
        railR->rotate(BRIDGE_ROT, 0.0f, 1.0f, 0.0f);
        railR->translate(BRIDGE_CX, BRIDGE_Y + 0.29f,
                         BRIDGE_CZ + cosf(glm::radians(BRIDGE_ROT)) * (BRIDGE_W * 0.5f - 0.1f));
        railR->setup();
        addMesh(railR);
    }

    // ── [15–17] Rock obstacles ────────────────────────────────────────────────
    {
        for (int ri = 0; ri < 2; ri++)
        {
            const H18Rock& r = ROCKS[ri];
            float baseY = r.topArc
                ? topArcHeight(r.arcT, r.radT)
                : botArcHeight(r.arcT, r.radT);

            Mesh* rock = new Mesh(Mesh::makeRock(r.scale));
            rock->translate(r.x-9, baseY-1 + r.scale * 0.5f, r.z);
            rock->rotate((float)ri * 61.0f, 0.0f, 1.0f, 0.0f);
            rock->setup();
            addMesh(rock);
        }
    }
}

    // ── [19] Lake water ───────────────────────────────────────────────────────
    {
        waterLake = new WaterMesh();
        waterLake->planeWidth     = LAKE_W;
        waterLake->planeLength    = LAKE_L;
        waterLake->gridResolution = 48;

        // Lake is calm — very gentle waves
        waterLake->waves[0] = { glm::vec2( 1.0f,  0.0f), 0.04f, 5.0f };
        waterLake->waves[1] = { glm::vec2( 0.0f,  1.0f), 0.03f, 4.0f };
        waterLake->waves[2] = { glm::vec2( 0.7f,  0.7f), 0.02f, 3.0f };
        waterLake->waves[3] = { glm::vec2(-0.5f,  0.5f), 0.015f,2.0f };

        waterLake->scrollSpeed0  = 0.03f;
        waterLake->scrollSpeed1  = 0.02f;
        waterLake->colorShallow  = glm::vec3(0.20f, 0.48f, 0.58f);
        waterLake->colorDeep     = glm::vec3(0.02f, 0.12f, 0.28f);
        waterLake->foamThreshold = 0.18f;   // almost no foam — still lake

        waterLake->modelMatrix =
            glm::translate(glm::mat4(1.0f),
                glm::vec3(LAKE_CX, LAKE_Y, LAKE_CZ));

        waterLake->init(
            "src/shaders/water.vert",
            "src/shaders/water.frag",
            "src/assets/textures/water_normal.bmp",
            "src/assets/textures/water_normal2.bmp",
            "src/assets/textures/water_foam.bmp");

        // Lake bank — low retaining wall around the lake pocket
        Mesh* bank = new Mesh(Mesh::makeCylinder(
            LAKE_W * 0.55f, 0.30f, 32,
            0.58f, 0.52f, 0.42f, 1.0f));
        bank->translate(LAKE_CX, LAKE_Y - 0.05f, LAKE_CZ);
        bank->setup();
        addMesh(bank);
    }

    {
        waterLake2 = new WaterMesh();
        waterLake2->planeWidth     = LAKE_W;
        waterLake2->planeLength    = LAKE_L;
        waterLake2->gridResolution = 48;

        // Lake is calm — very gentle waves
        waterLake2->waves[0] = { glm::vec2( 1.0f,  0.0f), 0.04f, 5.0f };
        waterLake2->waves[1] = { glm::vec2( 0.0f,  1.0f), 0.03f, 4.0f };
        waterLake2->waves[2] = { glm::vec2( 0.7f,  0.7f), 0.02f, 3.0f };
        waterLake2->waves[3] = { glm::vec2(-0.5f,  0.5f), 0.015f,2.0f };

        waterLake2->scrollSpeed0  = 0.03f;
        waterLake2->scrollSpeed1  = 0.02f;
        waterLake2->colorShallow  = glm::vec3(0.20f, 0.48f, 0.58f);
        waterLake2->colorDeep     = glm::vec3(0.02f, 0.12f, 0.28f);
        waterLake2->foamThreshold = 0.18f;   // almost no foam — still lake

        waterLake2->modelMatrix =
            glm::translate(glm::mat4(1.0f),
                glm::vec3(LAKE_CX+17, LAKE_Y, LAKE_CZ));

        waterLake2->init(
            "src/shaders/water.vert",
            "src/shaders/water.frag",
            "src/assets/textures/water_normal.bmp",
            "src/assets/textures/water_normal2.bmp",
            "src/assets/textures/water_foam.bmp");

        // Lake bank — low retaining wall around the lake pocket
        Mesh* bank = new Mesh(Mesh::makeCylinder(
            LAKE_W * 0.55f, 0.30f, 32,
            0.58f, 0.52f, 0.42f, 1.0f));
        bank->translate(LAKE_CX+17, LAKE_Y - 0.05f, LAKE_CZ);
        bank->setup();
        addMesh(bank);
    }
}

// =============================================================================
// Destructor
// =============================================================================
Hole18::~Hole18()
{
    for (int i = 0; i < MAX_MESHES; i++) {
        delete meshes[i];
        meshes[i] = nullptr;
    }
    delete waterLake;
    delete waterLake2;
}

void Hole18::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

// =============================================================================
// Per-frame
// =============================================================================
void Hole18::update(float deltaTime)
{
    if (waterLake)   waterLake->update(deltaTime);
    if (waterLake2)   waterLake2->update(deltaTime);
}

void Hole18::setCameraData(const glm::mat4& view,
                           const glm::mat4& proj,
                           const glm::vec3& pos)
{
    camView = view;
    camProj = proj;
    camPos  = pos;
}

// =============================================================================
// Draw
// =============================================================================
void Hole18::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);

    // Water last — alpha blended, depth mask off (handled inside WaterMesh)
    if (waterLake)   waterLake->draw(camView, camProj, camPos);
    if (waterLake2)   waterLake2->draw(camView, camProj, camPos);
}

void Hole18::drawDepth(GLuint depthShader)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->drawDepth(depthShader);
}

// =============================================================================
// Transforms — propagate to all meshes (water positions are baked into
// modelMatrix so they don't get double-transformed here)
// =============================================================================
void Hole18::translate(float x, float y, float z)
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    for (int i = 0; i < meshCount; i++)
        meshes[i]->applyMatrix(T);
}

void Hole18::rotate(float angle, float ax, float ay, float az)
{
    glm::mat4 R = glm::rotate(glm::mat4(1.0f),
                              glm::radians(angle),
                              glm::vec3(ax, ay, az));
    for (int i = 0; i < meshCount; i++)
        meshes[i]->applyMatrix(R);
}

void Hole18::scale(float sx, float sy, float sz)
{
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
    for (int i = 0; i < meshCount; i++)
        meshes[i]->applyMatrix(S);
}