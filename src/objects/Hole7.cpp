// Hole7.cpp  —  Swamp Hazard Hole (Hole 7)
// Dino Park Phuket — swamp & waterfall zone (Holes 5–8)
//
// STRICT FOOTPRINT: 28 (X) × 15 (Z), centred at local origin.
//   X runs from -14 to +14
//   Z runs from  -7.5 to +7.5
//
// ELEVATION: Entirely flat at Y = 0.
//
// LAYOUT (left→right, -X→+X):
//   Full-length fairway strip in the Z-centre, width = FAIRWAY_Z (6 units).
//   Swamp panels fill the remaining Z area on both sides of the fairway.
//   The fairway narrows slightly in the swamp zone (X in [-8, +8]) for challenge.
//
//   [-14,  -8]  Dry entry section — tee end, trees, rocks
//   [ -8,  +8]  Swamp zone — narrow fairway + swamp pools flanking it
//   [ +8, +14]  Dry exit section — cup/flag end, trees, rocks
//
// FEATURES (matching report Sections 2.1 & 5.3):
//   • Full-length narrow fairway slab with slightly narrowed swamp section
//   • Swamp pool: two murky-green semi-transparent panels (±Z of fairway)
//   • Mud bank strips between fairway edge and swamp water
//   • Bone obstacle lying across the fairway in the swamp zone
//   • Rock clusters at the swamp perimeter
//   • Trees at dry entry/exit ends
//   • Flowers along inner borders
//   • Cup + flag on +X exit end

#include "Hole7.h"
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

// Fairway widths
static const float FW_DRY   = HOLE_D;       // full width on dry ends
static const float FW_SWAMP = 6.0f;         // narrow path through swamp
static const float FW_HALF  = FW_SWAMP * 0.5f;  // 3.0

// Dry end extents
static const float DRY_X0 = -HALF_W;    // -14
static const float DRY_X1 = -8.0f;      // left dry section ends here
static const float SWAMP_X0 = -8.0f;
static const float SWAMP_X1 = +8.0f;
static const float DRY_X2 = +8.0f;
static const float DRY_X3 = +HALF_W;   // +14

static const float DRY_LEFT_W   = DRY_X1 - DRY_X0;    // 6
static const float SWAMP_ZONE_W = SWAMP_X1 - SWAMP_X0; // 16
static const float DRY_RIGHT_W  = DRY_X3 - DRY_X2;    // 6

static const float DRY_LEFT_CX   = DRY_X0  + DRY_LEFT_W   * 0.5f;  // -11
static const float SWAMP_ZONE_CX = (SWAMP_X0 + SWAMP_X1)   * 0.5f;  //   0
static const float DRY_RIGHT_CX  = DRY_X2  + DRY_RIGHT_W  * 0.5f;  // +11

// Swamp pool: flanks the narrow fairway in the swamp zone
// Fairway runs Z in [-FW_HALF, +FW_HALF], so swamp starts outside that
static const float MUD_W    = 1.0f;    // mud bank width in Z (between fair & water)
static const float SWAMP_Z0 = FW_HALF + MUD_W;   // inner edge of swamp water from Z=0
static const float SWAMP_ZW = HALF_D - SWAMP_Z0; // swamp water extent in Z

// Safe decoration margin
static const float MARGIN  = 1.2f;
static const float INNER_X = HALF_W - MARGIN;
static const float INNER_Z = HALF_D - MARGIN;

// ── Colours ───────────────────────────────────────────────────────────────────
// Fairway — darkening as we go deeper into the swamp zone
static const float GR = 0.10f, GG = 0.47f, GB = 0.13f;  // dry sections
static const float SGR= 0.09f, SGG= 0.40f, SGB= 0.11f;  // swamp-side strip (darker)

// Swamp water — murky green-brown (report: RGB 40,80,40, alpha 0.5)
static const float SWR = 0.16f, SWG = 0.31f, SWB = 0.16f;
static const float SWA = 0.55f;

// Mud banks — dark earthy brown
static const float MR = 0.28f, MG = 0.20f, MB = 0.10f;
// Mud in swamp — darker, more saturated greenish
static const float SMR= 0.22f, SMG= 0.18f, SMB= 0.08f;

// Concrete kerb
static const float CR = 0.86f, CG = 0.86f, CB = 0.86f;

// Bone — chalky off-white (matches Hole5 bone obstacle)
static const float BR = 0.92f, BG = 0.90f, BB = 0.82f;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void Hole7::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole7::addFlagAt(float x, float baseY, float z)
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

void Hole7::addFlowerAt(float x, float baseY, float z,
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

void Hole7::addTree(float x, float baseY, float z, float scale)
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

Hole7::Hole7() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    buildFairway();
    buildSwamp();
    buildBoneObstacle();
    buildRocks();
    buildFlag();
    buildTrees();
    buildFlowers();
}

Hole7::~Hole7()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFairway
// Three-section flat fairway:
//   1. Left dry section  (full HOLE_D width)
//   2. Narrow swamp path (FW_SWAMP width, centred in Z)
//   3. Right dry section (full HOLE_D width)
//
// Border kerb walls go around the whole 28×15 footprint as usual.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildFairway()
{
    // ── Border kerb walls ─────────────────────────────────────────────────────
    float wallXNeg = -HALF_W - BT * 0.5f;
    float wallXPos =  HALF_W + BT * 0.5f;
    float wallZNeg = -HALF_D - BT * 0.5f;
    float wallZPos =  HALF_D + BT * 0.5f;

    Mesh* bZPos = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CR, CG, CB, 1.0f));
    bZPos->translate(0.0f, BH * 0.5f, wallZPos);
    bZPos->setup(); addMesh(bZPos);

    Mesh* bZNeg = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CR, CG, CB, 1.0f));
    bZNeg->translate(0.0f, BH * 0.5f, wallZNeg);
    bZNeg->setup(); addMesh(bZNeg);

    Mesh* bXNeg = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT,
                                             CR, CG, CB, 1.0f));
    bXNeg->translate(wallXNeg, BH * 0.5f, 0.0f);
    bXNeg->setup(); addMesh(bXNeg);

    Mesh* bXPos = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT,
                                             CR, CG, CB, 1.0f));
    bXPos->translate(wallXPos, BH * 0.5f, 0.0f);
    bXPos->setup(); addMesh(bXPos);

    // ── Left dry section ──────────────────────────────────────────────────────
    Mesh* leftFW = new Mesh(Mesh::makeCuboid(DRY_LEFT_W, FH, FW_DRY,
                                              GR, GG, GB, 1.0f));
    leftFW->translate(DRY_LEFT_CX, FH * 0.5f, 0.0f);
    leftFW->setup(); addMesh(leftFW);

    // Earth under left section
    Mesh* leftEarth = new Mesh(Mesh::makeCuboid(DRY_LEFT_W, 0.5f, FW_DRY,
                                                  MR, MG, MB, 1.0f));
    leftEarth->translate(DRY_LEFT_CX, -0.25f, 0.0f);
    leftEarth->setup(); addMesh(leftEarth);

    // ── Swamp zone — narrow fairway path (centred in Z) ───────────────────────
    Mesh* swampFW = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, FH, FW_SWAMP,
                                               SGR, SGG, SGB, 1.0f));
    swampFW->translate(SWAMP_ZONE_CX, FH * 0.5f, 0.0f);
    swampFW->setup(); addMesh(swampFW);

    // Earth under swamp path
    Mesh* swampEarth = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, 0.5f, FW_SWAMP,
                                                   SMR, SMG, SMB, 1.0f));
    swampEarth->translate(SWAMP_ZONE_CX, -0.25f, 0.0f);
    swampEarth->setup(); addMesh(swampEarth);

    // ── Right dry section ─────────────────────────────────────────────────────
    Mesh* rightFW = new Mesh(Mesh::makeCuboid(DRY_RIGHT_W, FH, FW_DRY,
                                               GR * 0.95f, GG * 0.95f, GB, 1.0f));
    rightFW->translate(DRY_RIGHT_CX, FH * 0.5f, 0.0f);
    rightFW->setup(); addMesh(rightFW);

    // Earth under right section
    Mesh* rightEarth = new Mesh(Mesh::makeCuboid(DRY_RIGHT_W, 0.5f, FW_DRY,
                                                   MR, MG, MB, 1.0f));
    rightEarth->translate(DRY_RIGHT_CX, -0.25f, 0.0f);
    rightEarth->setup(); addMesh(rightEarth);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildSwamp
// Swamp pool flanking the narrow fairway strip in the swamp zone.
//
// From the report (Section 2.1 obstacle 7):
//   "A shallow swamp pool surrounds part of Hole 7's fairway, acting as an
//    out-of-bounds marker. Rendered as a flat plane with a murky-green
//    semi-transparent texture."
//
// Layout (for each ±Z side):
//   [mud bank strip] [swamp water panel] — from fairway edge to border wall
//
// The swamp ONLY covers the swamp zone in X [-8, +8], matching the
// narrow fairway section. Outside that (dry sections) the full-width
// fairway already covers Z.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildSwamp()
{
    // Mud bank Z positions (between fairway edge and swamp water)
    float mudCZ_pos = FW_HALF + MUD_W * 0.5f;   //  +Z mud bank centre
    float mudCZ_neg = -(FW_HALF + MUD_W * 0.5f); // -Z mud bank centre

    // Swamp water panel centre Z
    float swampCZ_pos =  SWAMP_Z0 + SWAMP_ZW * 0.5f;
    float swampCZ_neg = -(SWAMP_Z0 + SWAMP_ZW * 0.5f);

    // ── Mud banks (each side) ─────────────────────────────────────────────────
    Mesh* mudPos = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, FH * 0.6f, MUD_W,
                                              SMR, SMG, SMB, 1.0f));
    mudPos->translate(SWAMP_ZONE_CX, FH * 0.3f, mudCZ_pos);
    mudPos->setup(); addMesh(mudPos);

    Mesh* mudNeg = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, FH * 0.6f, MUD_W,
                                              SMR, SMG, SMB, 1.0f));
    mudNeg->translate(SWAMP_ZONE_CX, FH * 0.3f, mudCZ_neg);
    mudNeg->setup(); addMesh(mudNeg);

    // ── Swamp water panels — semi-transparent murky green ─────────────────────
    // Water surface flush with / just above ground (Y = 0.06)
    Mesh* waterPos = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, 0.10f, SWAMP_ZW,
                                                SWR, SWG, SWB, SWA));
    waterPos->translate(SWAMP_ZONE_CX, 0.05f, swampCZ_pos);
    waterPos->setup(); addMesh(waterPos);

    Mesh* waterNeg = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, 0.10f, SWAMP_ZW,
                                                SWR, SWG, SWB, SWA));
    waterNeg->translate(SWAMP_ZONE_CX, 0.05f, swampCZ_neg);
    waterNeg->setup(); addMesh(waterNeg);

    // ── Swamp bed (dark earth under the water panels, for visible depth) ──────
    Mesh* bedPos = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, 0.25f, SWAMP_ZW,
                                              SMR * 0.7f, SMG * 0.6f,
                                              SMB * 0.5f, 1.0f));
    bedPos->translate(SWAMP_ZONE_CX, -0.12f, swampCZ_pos);
    bedPos->setup(); addMesh(bedPos);

    Mesh* bedNeg = new Mesh(Mesh::makeCuboid(SWAMP_ZONE_W, 0.25f, SWAMP_ZW,
                                              SMR * 0.7f, SMG * 0.6f,
                                              SMB * 0.5f, 1.0f));
    bedNeg->translate(SWAMP_ZONE_CX, -0.12f, swampCZ_neg);
    bedNeg->setup(); addMesh(bedNeg);

    // ── Lily pad accents — small flat discs on the swamp surface ──────────────
    // Simple thin cylinders to suggest aquatic vegetation
    struct LilyDef { float x, z; };
    LilyDef lilies[] = {
        { -5.0f,  swampCZ_pos - 0.4f },
        { -1.5f,  swampCZ_pos + 0.3f },
        {  3.0f,  swampCZ_pos - 0.2f },
        {  6.0f,  swampCZ_pos + 0.5f },
        { -4.0f,  swampCZ_neg + 0.3f },
        {  1.5f,  swampCZ_neg - 0.4f },
        {  5.0f,  swampCZ_neg + 0.2f },
    };
    for (auto& ld : lilies)
    {
        Mesh* lily = new Mesh(Mesh::makeCylinder(0.45f, 0.06f, 10,
                                                  0.18f, 0.42f, 0.18f, 0.85f));
        lily->translate(ld.x, 0.11f, ld.z);
        lily->setup(); addMesh(lily);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// buildBoneObstacle
// Oversized dinosaur bone lying across the narrow fairway path, mid swamp zone.
// From the report (Section 2.1 obstacle 3):
//   "Elongated cylinders...chalky appearance."
// Bone runs in Z across the FW_SWAMP width.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildBoneObstacle()
{
    float boneX  = SWAMP_ZONE_CX - 1.0f;   // slightly left of centre
    float boneY  = FH + 0.35f;             // sitting on the fairway
    float shaftL = FW_SWAMP * 0.80f;       // shaft spans most of the path width

    // Shaft (horizontal cylinder running along Z)
    Mesh* shaft = new Mesh(Mesh::makeCylinder(0.20f, shaftL, 10,
                                               BR, BG, BB, 1.0f));
    shaft->rotate(90.0f, 1.0f, 0.0f, 0.0f);  // rotate to lie along Z
    shaft->translate(boneX, boneY, 0.0f);
    shaft->setup(); addMesh(shaft);

    // End knobs (bulbous joint cylinders)
    Mesh* j1 = new Mesh(Mesh::makeCylinder(0.42f, 0.48f, 12, BR, BG, BB, 1.0f));
    j1->rotate(90.0f, 1.0f, 0.0f, 0.0f);
    j1->translate(boneX, boneY,  shaftL * 0.48f);
    j1->setup(); addMesh(j1);

    Mesh* j2 = new Mesh(Mesh::makeCylinder(0.42f, 0.48f, 12, BR, BG, BB, 1.0f));
    j2->rotate(90.0f, 1.0f, 0.0f, 0.0f);
    j2->translate(boneX, boneY, -shaftL * 0.48f);
    j2->setup(); addMesh(j2);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildRocks
// Clusters of boulders at the swamp perimeter and along the dry section borders.
// The swamp edge rocks help define the boundary visually.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildRocks()
{
    struct RockDef { float x, z, s; };

    RockDef rocks[] = {
        // Left dry section — ±Z borders
        { -12.5f, -(INNER_Z - 0.5f), 0.82f },
        { -11.0f,  (INNER_Z - 0.5f), 0.78f },
        { -10.0f, -(INNER_Z - 0.6f), 0.70f },

        // Swamp perimeter — +Z side (flanking the swamp water)
        {  -6.0f, SWAMP_Z0 + SWAMP_ZW - 0.5f, 0.72f },
        {  -2.0f, SWAMP_Z0 + SWAMP_ZW - 0.6f, 0.65f },
        {   2.5f, SWAMP_Z0 + SWAMP_ZW - 0.5f, 0.68f },
        {   6.0f, SWAMP_Z0 + SWAMP_ZW - 0.4f, 0.74f },

        // Swamp perimeter — -Z side
        {  -5.5f, -(SWAMP_Z0 + SWAMP_ZW - 0.5f), 0.70f },
        {  -1.5f, -(SWAMP_Z0 + SWAMP_ZW - 0.6f), 0.66f },
        {   3.0f, -(SWAMP_Z0 + SWAMP_ZW - 0.5f), 0.72f },
        {   6.5f, -(SWAMP_Z0 + SWAMP_ZW - 0.4f), 0.75f },

        // Right dry section — ±Z borders
        {  11.0f, -(INNER_Z - 0.5f), 0.80f },
        {  12.5f,  (INNER_Z - 0.5f), 0.76f },
        {   9.5f,  (INNER_Z - 0.6f), 0.70f },
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
// Cup + flag on the right (exit) dry section.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildFlag()
{
    float flagX = DRY_RIGHT_CX + 1.5f;  // toward +X end
    float flagZ = 0.5f;

    Mesh* cup = new Mesh(Mesh::makeCylinder(0.48f, 0.28f, 12,
                                             0.10f, 0.10f, 0.10f, 1.0f));
    cup->translate(flagX, FH + 0.04f, flagZ);
    cup->setup(); addMesh(cup);

    addFlagAt(flagX, FH, flagZ);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildTrees
// Trees confined to the dry entry and exit sections where there is full width.
// Nothing in the swamp zone — the narrowness is the challenge.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildTrees()
{
    struct TreeDef { float x, z, s; };
    TreeDef trees[] = {
        // Left dry entry — corners
        { -13.0f, -5.5f, 0.82f },
        { -13.0f,  5.5f, 0.78f },
        { -10.5f, -5.0f, 0.75f },
        { -10.5f,  5.2f, 0.80f },

        // Right dry exit — corners
        {  13.0f, -5.5f, 0.85f },
        {  13.0f,  5.5f, 0.80f },
        {  10.5f, -5.0f, 0.72f },
        {  10.5f,  5.2f, 0.76f },
    };

    for (auto& td : trees)
        addTree(td.x, FH, td.z, td.s);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFlowers
// Flowers on the dry section inner borders only.
// ─────────────────────────────────────────────────────────────────────────────
void Hole7::buildFlowers()
{
    float baseY = FH;

    struct FlowerDef { float x, z; float r, g, b; };
    FlowerDef flowers[] = {
        // Left dry section
        { -13.0f, -4.0f, 1.00f, 0.75f, 0.10f },
        { -11.5f, -4.5f, 0.85f, 0.20f, 0.80f },
        { -13.0f,  4.2f, 0.90f, 0.85f, 0.10f },
        { -11.0f,  4.5f, 0.80f, 0.25f, 0.70f },

        // Right dry section
        {  13.0f, -4.2f, 1.00f, 0.40f, 0.10f },
        {  11.0f, -4.5f, 0.95f, 0.88f, 0.10f },
        {  13.0f,  4.0f, 0.95f, 0.30f, 0.60f },
        {  11.5f,  4.5f, 1.00f, 0.35f, 0.55f },

        // Near flag
        { DRY_RIGHT_CX + 0.5f,  2.0f, 0.95f, 0.88f, 0.10f },
        { DRY_RIGHT_CX - 1.2f, -2.2f, 1.00f, 0.35f, 0.55f },
    };

    for (auto& fd : flowers)
        addFlowerAt(fd.x, baseY, fd.z, fd.r, fd.g, fd.b);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

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

void Hole7::draw(GLuint shader, bool wireframe,
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