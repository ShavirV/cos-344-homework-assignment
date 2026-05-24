// Hole6.cpp  —  Bridge & Waterfall Crossing (Hole 6)
// Dino Park Phuket — swamp & waterfall zone (Holes 5–8)
//
// STRICT FOOTPRINT: 28 (X) × 15 (Z), centred at local origin.
//   X runs from -14 to +14
//   Z runs from  -7.5 to +7.5
//
// ELEVATION: Entirely flat at Y = 0 (no height change on this hole).
//            Bridge Y-surface is FH (0.2) — same height as the fairway slab.
//
// LAYOUT (left→right, -X→+X):
//   [-14, -4]  Left fairway  — tee end, sign post, rocks, trees
//   [ -4, +4]  Stream channel + bridge spanning it
//   [ +4, +14] Right fairway — cup/flag end, rocks, trees
//
// FEATURES (matching report Section 2.1 & 5.3):
//   • Flat green fairway slab (earth fill beneath)
//   • Narrow stream channel (water strip at Z-centre, running X = -4 to +4)
//   • Wooden bridge: deck planks + vertical railing posts + horizontal handrail
//   • Directional sign post near tee (-X end)
//   • Rock formations along ±Z borders
//   • Trees inside corner margins
//   • Tropical flowers along inner edges
//   • Cup + flag on right pad

#include "Hole6.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Dimension constants (local space)
// ─────────────────────────────────────────────────────────────────────────────

static const float HOLE_W  = 28.0f;
static const float HOLE_D  = 15.0f;
static const float HALF_W  = 14.0f;
static const float HALF_D  = 7.5f;

static const float FH  = 0.2f;    // fairway slab height
static const float BT  = 0.5f;    // border wall thickness
static const float BH  = 0.4f;    // border kerb height

// Stream / bridge zone
static const float STREAM_X0 = -4.0f;  // stream start (left edge)
static const float STREAM_X1 =  4.0f;  // stream end   (right edge)
static const float STREAM_W  = STREAM_X1 - STREAM_X0;   // 8.0
static const float STREAM_Z  = 2.8f;   // half-width of stream in Z

// Bridge
static const float BRIDGE_W  = STREAM_W;   // bridge length in X = 8
static const float BRIDGE_D  = 4.0f;       // bridge deck width in Z
static const float BRIDGE_H  = FH + 0.05f; // bridge surface just above fairway

// Left / right fairway sub-widths
static const float LEFT_W   = HALF_W + STREAM_X0;   // 14 + (-4) = 10
static const float RIGHT_W  = HALF_W - STREAM_X1;   // 14 - 4    = 10
static const float LEFT_CX  = -HALF_W + LEFT_W  * 0.5f;  // -9
static const float RIGHT_CX =  HALF_W - RIGHT_W * 0.5f;  //  9

// Safe inner margin for decorations
static const float MARGIN  = 1.2f;
static const float INNER_X = HALF_W - MARGIN;
static const float INNER_Z = HALF_D - MARGIN;

// ── Colours ───────────────────────────────────────────────────────────────────
// Fairway greens (slightly darker than Hole5 — deeper in the swamp zone)
static const float GR = 0.11f, GG = 0.52f, GB = 0.13f;

// Wood (bridge planks & posts) — warm brown
static const float WDR = 0.45f, WDG = 0.28f, WDB = 0.10f;
// Handrail — slightly lighter
static const float HRR = 0.55f, HRG = 0.35f, HRB = 0.15f;

// Concrete kerb
static const float CR = 0.86f, CG = 0.86f, CB = 0.86f;

// Water / stream — blue-green (report: RGB 30,100,120, alpha 0.6)
static const float WTR_R = 0.12f, WTR_G = 0.39f, WTR_B = 0.47f;
static const float WTR_A = 0.62f;

// Earth / mud beneath
static const float ER = 0.28f, EG = 0.20f, EB = 0.10f;

// Sign post — wood brown
static const float SPR = 0.40f, SPG = 0.25f, SPB = 0.08f;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void Hole6::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

// ── addFlagAt ─────────────────────────────────────────────────────────────────
void Hole6::addFlagAt(float x, float baseY, float z)
{
    Mesh* pole = new Mesh(Mesh::makeCylinder(0.10f, 4.5f, 8,
                                              0.85f, 0.85f, 0.85f, 1.0f));
    pole->translate(x, baseY + 2.25f, z);
    pole->setup(); addMesh(pole);

    Mesh* pennant = new Mesh(Mesh::makeTriangularPrism(1.6f, 0.85f, 0.12f,
                                                        0.9f, 0.08f, 0.08f, 1.0f));
    pennant->translate(x + 0.8f, baseY + 4.4f, z);
    pennant->setup(); addMesh(pennant);
}

// ── addFlowerAt ───────────────────────────────────────────────────────────────
void Hole6::addFlowerAt(float x, float baseY, float z,
                         float pr, float pg, float pb)
{
    Mesh* stem = new Mesh(Mesh::makeCylinder(0.055f, 0.55f, 6,
                                              0.2f, 0.55f, 0.1f, 1.0f));
    stem->translate(x, baseY + 0.275f, z);
    stem->setup(); addMesh(stem);

    Mesh* petal = new Mesh(Mesh::makeCone(0.32f, 0.48f, 8, pr, pg, pb, 1.0f));
    petal->translate(x, baseY + 0.70f, z);
    petal->setup(); addMesh(petal);

    Mesh* centre = new Mesh(Mesh::makeCylinder(0.11f, 0.12f, 8,
                                                1.0f, 0.88f, 0.1f, 1.0f));
    centre->translate(x, baseY + 0.98f, z);
    centre->setup(); addMesh(centre);
}

// ── addTree ───────────────────────────────────────────────────────────────────
void Hole6::addTree(float x, float baseY, float z, float scale)
{
    float s = scale;
    Mesh* trunk = new Mesh(Mesh::makeCylinder(0.32f * s, 4.0f * s, 8,
                                               0.44f, 0.29f, 0.11f, 1.0f));
    trunk->translate(x, baseY + 2.0f * s, z);
    trunk->setup(); addMesh(trunk);

    Mesh* c1 = new Mesh(Mesh::makeCone(2.4f * s, 3.2f * s, 10,
                                        0.10f, 0.44f, 0.14f, 1.0f));
    c1->translate(x, baseY + 5.0f * s, z);
    c1->setup(); addMesh(c1);

    Mesh* c2 = new Mesh(Mesh::makeCone(1.5f * s, 2.2f * s, 10,
                                        0.12f, 0.50f, 0.18f, 1.0f));
    c2->translate(x, baseY + 7.0f * s, z);
    c2->setup(); addMesh(c2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Hole6::Hole6() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    buildFairway();
    buildStream();
    buildBridge();
    buildSignPost();
    buildRocks();
    buildFlag();
    buildTrees();
    buildFlowers();
}

Hole6::~Hole6()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFairway
// Left pad + right pad + border kerb walls.
// No centre slab — the stream channel occupies X in [STREAM_X0, STREAM_X1].
// Earth fill under both pads so the sides look solid.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildFairway()
{
    // ── Border kerb walls ─────────────────────────────────────────────────────
    float wallXNeg = -HALF_W - BT * 0.5f;
    float wallXPos =  HALF_W + BT * 0.5f;
    float wallZNeg = -HALF_D - BT * 0.5f;
    float wallZPos =  HALF_D + BT * 0.5f;

    // +Z wall
    Mesh* bZPos = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CR, CG, CB, 1.0f));
    bZPos->translate(0.0f, BH * 0.5f, wallZPos);
    bZPos->setup(); addMesh(bZPos);

    // -Z wall
    Mesh* bZNeg = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CR, CG, CB, 1.0f));
    bZNeg->translate(0.0f, BH * 0.5f, wallZNeg);
    bZNeg->setup(); addMesh(bZNeg);

    // -X wall
    Mesh* bXNeg = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT,
                                             CR, CG, CB, 1.0f));
    bXNeg->translate(wallXNeg, BH * 0.5f, 0.0f);
    bXNeg->setup(); addMesh(bXNeg);

    // +X wall
    Mesh* bXPos = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT,
                                             CR, CG, CB, 1.0f));
    bXPos->translate(wallXPos, BH * 0.5f, 0.0f);
    bXPos->setup(); addMesh(bXPos);

    // ── Left fairway slab ─────────────────────────────────────────────────────
    Mesh* leftFW = new Mesh(Mesh::makeCuboid(LEFT_W, FH, HOLE_D, GR, GG, GB, 1.0f));
    leftFW->translate(LEFT_CX, FH * 0.5f, 0.0f);
    leftFW->setup(); addMesh(leftFW);

    // Earth under left pad
    Mesh* leftEarth = new Mesh(Mesh::makeCuboid(LEFT_W, 0.5f, HOLE_D,
                                                 ER, EG, EB, 1.0f));
    leftEarth->translate(LEFT_CX, -0.25f, 0.0f);
    leftEarth->setup(); addMesh(leftEarth);

    // ── Right fairway slab ────────────────────────────────────────────────────
    Mesh* rightFW = new Mesh(Mesh::makeCuboid(RIGHT_W, FH, HOLE_D,
                                               GR * 0.95f, GG * 0.95f, GB, 1.0f));
    rightFW->translate(RIGHT_CX, FH * 0.5f, 0.0f);
    rightFW->setup(); addMesh(rightFW);

    // Earth under right pad
    Mesh* rightEarth = new Mesh(Mesh::makeCuboid(RIGHT_W, 0.5f, HOLE_D,
                                                   ER, EG, EB, 1.0f));
    rightEarth->translate(RIGHT_CX, -0.25f, 0.0f);
    rightEarth->setup(); addMesh(rightEarth);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildStream
// Narrow water strip running through the centre of the hole in X.
// The stream runs the full Z width of the hole.
// A shallow mud/earth bed beneath the water surface.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildStream()
{
    // Mud bed below the water surface — slightly depressed
    Mesh* bed = new Mesh(Mesh::makeCuboid(STREAM_W, 0.30f, HOLE_D,
                                           ER * 0.75f, EG * 0.6f, EB * 0.5f, 1.0f));
    bed->translate(0.0f, -0.15f, 0.0f);
    bed->setup(); addMesh(bed);

    // Water surface plane — semi-transparent blue-green
    // Full Z width so stream extends to the walls on both sides
    Mesh* water = new Mesh(Mesh::makeCuboid(STREAM_W, 0.08f, HOLE_D,
                                             WTR_R, WTR_G, WTR_B, WTR_A));
    water->translate(0.0f, 0.04f, 0.0f);
    water->setup(); addMesh(water);

    // Shallow river-stone strip along +Z bank (visual texture detail)
    Mesh* bankN = new Mesh(Mesh::makeCuboid(STREAM_W, 0.12f, 1.0f,
                                             0.50f, 0.48f, 0.45f, 1.0f));
    bankN->translate(0.0f, 0.06f, HALF_D - 0.5f);
    bankN->setup(); addMesh(bankN);

    // Shallow river-stone strip along -Z bank
    Mesh* bankS = new Mesh(Mesh::makeCuboid(STREAM_W, 0.12f, 1.0f,
                                             0.50f, 0.48f, 0.45f, 1.0f));
    bankS->translate(0.0f, 0.06f, -(HALF_D - 0.5f));
    bankS->setup(); addMesh(bankS);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildBridge
// Wooden bridge crossing the stream channel.
//
// From the report (Section 2.1.1 "Wooden Bridge"):
//   "A flat rectangular quad forms the deck surface and 5-6 thinner rectangular
//    quads are laid across it, slightly raised in the Y-value, to make it appear
//    as if it is planks. On each long side, 2 thin cylinders serve as the railing
//    posts and a horizontal cylinder connects them as the handrail."
//
// Bridge sits at X in [STREAM_X0, STREAM_X1], Z in [-BRIDGE_D/2, +BRIDGE_D/2].
// Top surface at Y = BRIDGE_H so it floats just above the fairway.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildBridge()
{
    const float DECK_Y    = BRIDGE_H;                  // top of deck
    const float DECK_THK  = 0.14f;                     // deck slab thickness
    const float PLANK_THK = 0.05f;                     // plank strip thickness
    const float PLANK_W   = 0.55f;                     // each plank strip width in X
    const int   N_PLANKS  = 6;
    const float POST_R    = 0.09f;                     // railing post radius
    const float POST_H    = 1.10f;                     // railing post height
    const float RAIL_R    = 0.055f;                    // handrail radius
    const float POST_X0   = STREAM_X0 + 0.35f;        // first post X
    const float POST_X1   = STREAM_X1 - 0.35f;        // last  post X
    const float RAIL_LEN  = POST_X1 - POST_X0 + 2.0f * POST_R; // handrail spans posts

    // ── Deck slab ─────────────────────────────────────────────────────────────
    Mesh* deck = new Mesh(Mesh::makeCuboid(BRIDGE_W, DECK_THK, BRIDGE_D,
                                            WDR, WDG, WDB, 1.0f));
    deck->translate(0.0f, DECK_Y - DECK_THK * 0.5f, 0.0f);
    deck->setup(); addMesh(deck);

    // ── Planks (thin strips across the deck in Z, spaced along X) ─────────────
    float plankStep = (BRIDGE_W - PLANK_W) / (float)(N_PLANKS - 1);
    for (int i = 0; i < N_PLANKS; i++)
    {
        float px = STREAM_X0 + PLANK_W * 0.5f + i * plankStep;
        // Alternate plank shade slightly for visual separation
        float shade = (i % 2 == 0) ? 1.0f : 0.88f;
        Mesh* plank = new Mesh(Mesh::makeCuboid(PLANK_W, PLANK_THK, BRIDGE_D,
                                                 WDR * shade, WDG * shade,
                                                 WDB * shade, 1.0f));
        plank->translate(px, DECK_Y + PLANK_THK * 0.5f, 0.0f);
        plank->setup(); addMesh(plank);
    }

    // ── Railing posts — 2 posts per side, 4 total ─────────────────────────────
    // Post positions: left quarter and right quarter of the bridge span
    float postXPositions[2] = { POST_X0, POST_X1 };
    float railZSides[2]     = { -(BRIDGE_D * 0.5f - POST_R),
                                  (BRIDGE_D * 0.5f - POST_R) };

    for (int side = 0; side < 2; side++)
    {
        for (int p = 0; p < 2; p++)
        {
            Mesh* post = new Mesh(Mesh::makeCylinder(POST_R, POST_H, 8,
                                                      HRR, HRG, HRB, 1.0f));
            post->translate(postXPositions[p],
                            DECK_Y + POST_H * 0.5f,
                            railZSides[side]);
            post->setup(); addMesh(post);
        }

        // Handrail: horizontal cylinder connecting the two posts on this side
        Mesh* rail = new Mesh(Mesh::makeCylinder(RAIL_R, RAIL_LEN, 8,
                                                  HRR * 1.1f, HRG * 1.1f,
                                                  HRB * 1.1f, 1.0f));
        // Rotate 90° around Z so the cylinder lies along X
        rail->rotate(90.0f, 0.0f, 0.0f, 1.0f);
        rail->translate((POST_X0 + POST_X1) * 0.5f,
                        DECK_Y + POST_H,
                        railZSides[side]);
        rail->setup(); addMesh(rail);
    }

    // ── Structural support beams under deck (visible from the side) ────────────
    // Two horizontal beams running the full bridge length below the deck
    float beamZPositions[2] = { -(BRIDGE_D * 0.5f - 0.4f),
                                  (BRIDGE_D * 0.5f - 0.4f) };
    for (int b = 0; b < 2; b++)
    {
        Mesh* beam = new Mesh(Mesh::makeCuboid(BRIDGE_W, 0.12f, 0.18f,
                                                WDR * 0.80f, WDG * 0.80f,
                                                WDB * 0.80f, 1.0f));
        beam->translate(0.0f, DECK_Y - DECK_THK - 0.06f, beamZPositions[b]);
        beam->setup(); addMesh(beam);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildSignPost
// Directional sign post near the tee (left/−X end).
// From the report (Section 2.1.1 "Directional Sign Post"):
//   "A single tall thin rectangle will form the base of the post and at the
//    top, a smaller rectangle is attached horizontally as the sign board, and
//    an equilateral triangle is attached to the left side of the rectangle to
//    form the arrow pointer."
//
// Implemented as: vertical cuboid post + horizontal sign board cuboid
//                 + triangular prism arrow head.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildSignPost()
{
    const float POST_X = -12.0f;
    const float POST_Z = -4.5f;
    const float POST_W = 0.18f;
    const float POST_H = 3.2f;
    const float BOARD_W = 2.0f;
    const float BOARD_H = 0.70f;
    const float BOARD_D = 0.15f;
    const float ARR_W   = 0.70f;

    // Vertical post
    Mesh* post = new Mesh(Mesh::makeCuboid(POST_W, POST_H, POST_W,
                                            SPR, SPG, SPB, 1.0f));
    post->translate(POST_X, POST_H * 0.5f, POST_Z);
    post->setup(); addMesh(post);

    // Horizontal sign board
    Mesh* board = new Mesh(Mesh::makeCuboid(BOARD_W, BOARD_H, BOARD_D,
                                             SPR * 1.2f, SPG * 1.15f,
                                             SPB * 1.1f, 1.0f));
    board->translate(POST_X + BOARD_W * 0.5f - POST_W * 0.5f,
                     POST_H - BOARD_H * 0.5f - 0.1f,
                     POST_Z);
    board->setup(); addMesh(board);

    // Arrow tip: triangular prism rotated to point in +X direction
    // makeTriangularPrism produces a prism with triangle in XY plane, depth in Z.
    // We rotate 90° around Y to make it face +X, then 90° around X to lay it flat.
    Mesh* arrow = new Mesh(Mesh::makeTriangularPrism(ARR_W, BOARD_H * 0.85f,
                                                      BOARD_D,
                                                      0.9f, 0.15f, 0.10f, 1.0f));
    // Rotate so triangle points in +X
    arrow->rotate(90.0f, 0.0f, 1.0f, 0.0f);
    arrow->translate(POST_X + BOARD_W + ARR_W * 0.25f - POST_W * 0.5f,
                     POST_H - BOARD_H * 0.5f - 0.1f,
                     POST_Z);
    arrow->setup(); addMesh(arrow);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildRocks
// Volcanic boulders along the inner ±Z borders and flanking the stream banks.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildRocks()
{
    struct RockDef { float x, z, s; };

    RockDef rocks[] = {
        // -Z border — left fairway
        { -12.0f, -(INNER_Z - 0.5f), 0.82f },
        {  -8.5f, -(INNER_Z - 0.6f), 0.70f },
        {  -5.5f, -(INNER_Z - 0.4f), 0.90f },

        // +Z border — left fairway
        { -11.5f,  (INNER_Z - 0.6f), 0.75f },
        {  -7.0f,  (INNER_Z - 0.5f), 0.85f },

        // Stream banks — flanking the bridge approach
        {  STREAM_X0 - 0.8f, -(STREAM_Z + 0.4f), 0.65f },
        {  STREAM_X0 - 0.8f,  (STREAM_Z + 0.4f), 0.68f },
        {  STREAM_X1 + 0.8f, -(STREAM_Z + 0.4f), 0.62f },
        {  STREAM_X1 + 0.8f,  (STREAM_Z + 0.4f), 0.60f },

        // -Z border — right fairway
        {  8.0f, -(INNER_Z - 0.5f), 0.78f },
        { 11.5f, -(INNER_Z - 0.6f), 0.72f },

        // +Z border — right fairway
        {  7.5f,  (INNER_Z - 0.5f), 0.80f },
        { 12.0f,  (INNER_Z - 0.6f), 0.74f },
    };

    for (auto& rd : rocks)
    {
        Mesh* rock = new Mesh(Mesh::makeRock(rd.s));
        rock->translate(rd.x, rd.s * 1.0f, rd.z);
        rock->setup(); addMesh(rock);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFlag
// Cup + flag on the right pad (+X end).
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildFlag()
{
    float flagX = RIGHT_CX + 2.5f;   // toward +X end
    float flagZ = 0.5f;

    Mesh* cup = new Mesh(Mesh::makeCylinder(0.48f, 0.28f, 12,
                                             0.10f, 0.10f, 0.10f, 1.0f));
    cup->translate(flagX, FH + 0.04f, flagZ);
    cup->setup(); addMesh(cup);

    addFlagAt(flagX, FH, flagZ);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildTrees
// Palm-style trees in the four corner areas, well inside footprint.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildTrees()
{
    struct TreeDef { float x, z, s; };
    TreeDef trees[] = {
        // Left fairway — corner trees
        { -12.5f, -5.0f, 0.82f },
        { -12.0f,  5.2f, 0.78f },
        {  -9.0f, -5.5f, 0.75f },

        // Right fairway — corner trees
        {  11.5f, -5.0f, 0.85f },
        {  11.0f,  5.2f, 0.80f },
        {   8.0f,  5.5f, 0.72f },
        {   8.5f, -5.4f, 0.76f },
    };

    for (auto& td : trees)
        addTree(td.x, FH, td.z, td.s);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFlowers
// Scattered along the inner border edges of both fairways.
// ─────────────────────────────────────────────────────────────────────────────
void Hole6::buildFlowers()
{
    float baseY = FH;

    struct FlowerDef { float x, z; float r, g, b; };
    FlowerDef flowers[] = {
        // Left fairway -Z edge
        { -13.0f, -4.2f, 1.00f, 0.75f, 0.10f },
        { -10.0f, -4.5f, 0.95f, 0.30f, 0.60f },
        {  -6.5f, -4.0f, 0.85f, 0.20f, 0.80f },

        // Left fairway +Z edge
        { -11.5f,  4.0f, 0.90f, 0.85f, 0.10f },
        {  -8.0f,  4.5f, 0.80f, 0.25f, 0.70f },

        // Right fairway -Z edge
        {   6.5f, -4.2f, 1.00f, 0.40f, 0.10f },
        {   9.5f, -4.5f, 0.95f, 0.88f, 0.10f },

        // Right fairway +Z edge
        {   7.0f,  4.2f, 1.00f, 0.35f, 0.55f },
        {  10.5f,  4.5f, 0.90f, 0.30f, 0.60f },

        // Near flag
        { RIGHT_CX + 1.0f,  2.2f, 0.95f, 0.88f, 0.10f },
        { RIGHT_CX - 1.0f, -1.8f, 1.00f, 0.35f, 0.55f },
    };

    for (auto& fd : flowers)
        addFlowerAt(fd.x, baseY, fd.z, fd.r, fd.g, fd.b);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

void Hole6::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole6::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}

void Hole6::draw(GLuint shader, bool wireframe,
                 const glm::vec3& lightDir,
                 const glm::vec3& lightColor,
                 const glm::vec3& camPos)
{
    glUseProgram(shader);
    GLint locLD = glGetUniformLocation(shader, "uLightDir");
    GLint locVP = glGetUniformLocation(shader, "uViewPos");
    GLint locLC = glGetUniformLocation(shader, "uLightColor");
    if (locLD != -1) glUniform3fv(locLD, 1, glm::value_ptr(glm::normalize(lightDir)));
    if (locVP != -1) glUniform3fv(locVP, 1, glm::value_ptr(camPos));
    if (locLC != -1) glUniform3fv(locLC, 1, glm::value_ptr(lightColor));

    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shader, wireframe);
}