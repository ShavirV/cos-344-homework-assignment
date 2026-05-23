// =============================================================================
// Hole15.cpp
// Hole 15 — The Octopus Head
//
// Layout (top-down, X right, Z into screen):
//
//   [TEE]---[right corridor]---[  MANTLE ELLIPSE  ]---[left corridor]---[CUP]
//
//   • The MANTLE is a large elliptical slab (the "bulge" head).
//   • The RIGHT CORRIDOR is a short straight slab that sticks out of the
//     right (+X) end of the mantle — this is where the player tees off.
//   • The LEFT CORRIDOR is a short straight slab sticking out of the left
//     (−X) end — the player must putt the ball into the cup at its far end.
//   • Both corridors are the same width and thickness as the mantle slab.
//   • Concrete kerbs run along both long sides of each corridor and fully
//     around the mantle perimeter (with gaps at the corridor junctions).
//   • Two decorative eyes sit on the mantle top face.
// =============================================================================

#include "Hole15.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>

// =============================================================================
// Constants
// =============================================================================

static constexpr float PI = 3.14159265358979f;

// Mantle (head ellipse)
static constexpr float MANTLE_RX   = 3.0f;   // half-width  (X)
static constexpr float MANTLE_RZ   = 2.2f;   // half-depth  (Z)
static constexpr int   MANTLE_SEGS = 64;
static constexpr float SLAB_THICK  = 0.18f;

// Corridors (tee on right, cup on left)
static constexpr float CORR_HALF_W = 0.65f;  // half-width of corridor
static constexpr float CORR_LENGTH = 2.2f;   // how far they stick out from ellipse edge

// Kerbs
static constexpr float KERB_H = 0.12f;
static constexpr float KERB_T = 0.12f;

// Cup
static constexpr float CUP_HALF = 0.22f;

// Flag
static constexpr float FLAG_POLE_H = 1.2f;
static constexpr float FLAG_POLE_R = 0.035f;

// =============================================================================
// Colours
// =============================================================================

static const glm::vec3 COL_TURF      { 0.20f, 0.55f, 0.18f };
static const glm::vec3 COL_TURF_SIDE { 0.16f, 0.45f, 0.14f };
static const glm::vec3 COL_KERB      { 0.70f, 0.70f, 0.68f };
static const glm::vec3 COL_KERB_SIDE { 0.58f, 0.58f, 0.56f };
static const glm::vec3 COL_TEE       { 0.10f, 0.30f, 0.10f };
static const glm::vec3 COL_CUP       { 0.08f, 0.08f, 0.08f };
static const glm::vec3 COL_FLAG      { 1.00f, 0.15f, 0.10f };
static const glm::vec3 COL_POLE      { 0.90f, 0.90f, 0.90f };
static const glm::vec3 COL_EYE_S     { 0.95f, 0.95f, 0.90f };
static const glm::vec3 COL_EYE_P     { 0.05f, 0.05f, 0.08f };

// =============================================================================
// Constructor / Destructor
// =============================================================================

Hole15::Hole15()
    : mVAO(0), mVBO(0), mEBO(0)
    , mIndexCount(0)
    , mInitialised(false)
    , mProgram(0)
    , mLocMVP(-1), mLocModel(-1), mLocNormalMat(-1)
    , mLocLightDir(-1), mLocLightColor(-1), mLocAmbient(-1), mLocViewPos(-1)
    , mCenter(0.0f, 0.5f, 0.0f)
    , mRadius(8.0f)
{}

Hole15::~Hole15()
{
    cleanup();
}

// =============================================================================
// pushQuad — flat-normal quad (a-b-c-d, CCW from normal side)
// =============================================================================

void Hole15::pushQuad(const glm::vec3& a, const glm::vec3& b,
                      const glm::vec3& c, const glm::vec3& d,
                      const glm::vec3& n, const glm::vec3& colour)
{
    unsigned int base = static_cast<unsigned int>(mVertices.size());
    mVertices.push_back({ a, n, colour });
    mVertices.push_back({ b, n, colour });
    mVertices.push_back({ c, n, colour });
    mVertices.push_back({ d, n, colour });
    mIndices.push_back(base + 0); mIndices.push_back(base + 1); mIndices.push_back(base + 2);
    mIndices.push_back(base + 0); mIndices.push_back(base + 2); mIndices.push_back(base + 3);
}

// =============================================================================
// buildBox — axis-aligned box
// =============================================================================

void Hole15::buildBox(const glm::vec3& mn, const glm::vec3& mx,
                      const glm::vec3& colour)
{
    glm::vec3 x { mx.x - mn.x, 0, 0 };
    glm::vec3 y { 0, mx.y - mn.y, 0 };
    glm::vec3 z { 0, 0, mx.z - mn.z };

    glm::vec3 v000 = mn;
    glm::vec3 v100 = mn + x;
    glm::vec3 v010 = mn + y;
    glm::vec3 v110 = mn + x + y;
    glm::vec3 v001 = mn + z;
    glm::vec3 v101 = mn + x + z;
    glm::vec3 v011 = mn + y + z;
    glm::vec3 v111 = mn + x + y + z;

    pushQuad(v010, v110, v111, v011, { 0,  1,  0}, colour); // top
    pushQuad(v000, v001, v101, v100, { 0, -1,  0}, colour); // bottom
    pushQuad(v100, v101, v111, v110, { 1,  0,  0}, colour); // +X
    pushQuad(v000, v010, v011, v001, {-1,  0,  0}, colour); // -X
    pushQuad(v001, v011, v111, v101, { 0,  0,  1}, colour); // +Z
    pushQuad(v000, v100, v110, v010, { 0,  0, -1}, colour); // -Z
}

// =============================================================================
// buildMantle — large elliptical slab (top fan + side ring + bottom fan)
// The slab occupies y ∈ [-SLAB_THICK, 0].
// Gap angles skip the openings where the corridors attach (+X and -X).
// =============================================================================

void Hole15::buildMantle(float rx, float rz, float thick, int segs,
                         const glm::vec3& topColour,
                         const glm::vec3& sideColour)
{
    float y0 = -thick;
    float y1 =  0.0f;

    glm::vec3 nUp   { 0,  1, 0 };
    glm::vec3 nDown { 0, -1, 0 };

    // Angle of the corridor opening on each side (half-angle in radians).
    // The corridors are CORR_HALF_W wide, attached at x = ±MANTLE_RX.
    // At the attachment point the chord half-angle ≈ asin(CORR_HALF_W / rz)
    // but we use a slightly larger gap so the kerb doesn't poke into the slab.
    float gapHalf = std::asin(std::min(CORR_HALF_W / rz, 0.999f)) + 0.04f;

    for (int i = 0; i < segs; ++i)
    {
        float a0 = 2.0f * PI * float(i)     / float(segs);
        float a1 = 2.0f * PI * float(i + 1) / float(segs);
        float amid = 0.5f * (a0 + a1);

        // Normalise amid to [0, 2π)
        float am = std::fmod(amid + 4.0f * PI, 2.0f * PI);

        // Skip segments near angle 0 (right, +X, tee) and near angle π (left, -X, cup)
        float distRight = std::min(am, 2.0f * PI - am);          // dist from 0
        float distLeft  = std::fabs(am - PI);                     // dist from π

        bool inGap = (distRight < gapHalf) || (distLeft < gapHalf);

        // Top triangle
        glm::vec3 tA { rx * std::cos(a0), y1, rz * std::sin(a0) };
        glm::vec3 tB { rx * std::cos(a1), y1, rz * std::sin(a1) };
        glm::vec3 tC { 0.0f, y1, 0.0f };

        unsigned int base = static_cast<unsigned int>(mVertices.size());
        mVertices.push_back({ tA, nUp, topColour });
        mVertices.push_back({ tB, nUp, topColour });
        mVertices.push_back({ tC, nUp, topColour });
        mIndices.push_back(base); mIndices.push_back(base+1); mIndices.push_back(base+2);

        // Bottom triangle (reversed winding)
        glm::vec3 bA { rx * std::cos(a0), y0, rz * std::sin(a0) };
        glm::vec3 bB { rx * std::cos(a1), y0, rz * std::sin(a1) };
        glm::vec3 bC { 0.0f, y0, 0.0f };
        base = static_cast<unsigned int>(mVertices.size());
        mVertices.push_back({ bA, nDown, sideColour });
        mVertices.push_back({ bB, nDown, sideColour });
        mVertices.push_back({ bC, nDown, sideColour });
        mIndices.push_back(base); mIndices.push_back(base+2); mIndices.push_back(base+1);

        // Side quad (skip in gap so corridor opening is visible)
        if (!inGap)
        {
            glm::vec3 outN = glm::normalize(glm::vec3(std::cos(amid), 0.0f, std::sin(amid)));
            pushQuad(tA, bA, bB, tB, outN, sideColour);
        }
    }
}

// =============================================================================
// buildMantleKerb — kerb ring around mantle, with gaps at corridor openings
// =============================================================================

void Hole15::buildMantleKerb(float rx, float rz, int segs,
                              float kerbH, float kerbT,
                              const glm::vec3& colour)
{
    float gapHalf = std::asin(std::min(CORR_HALF_W / rz, 0.999f)) + 0.06f;

    for (int i = 0; i < segs; ++i)
    {
        float a0 = 2.0f * PI * float(i)     / float(segs);
        float a1 = 2.0f * PI * float(i + 1) / float(segs);
        float amid = 0.5f * (a0 + a1);
        float am   = std::fmod(amid + 4.0f * PI, 2.0f * PI);

        float distRight = std::min(am, 2.0f * PI - am);
        float distLeft  = std::fabs(am - PI);
        if (distRight < gapHalf || distLeft < gapHalf) continue;

        glm::vec3 outN = glm::normalize(glm::vec3(std::cos(amid), 0.0f, std::sin(amid)));

        glm::vec3 iA { rx * std::cos(a0), 0.0f, rz * std::sin(a0) };
        glm::vec3 iB { rx * std::cos(a1), 0.0f, rz * std::sin(a1) };
        glm::vec3 oA = iA + outN * kerbT;
        glm::vec3 oB = iB + outN * kerbT;

        // Top
        pushQuad(iA, oA, oB + glm::vec3(0, kerbH, 0), iB + glm::vec3(0, kerbH, 0),
                 {0,1,0}, colour);
        // Outer face
        pushQuad(oA, oA + glm::vec3(0,kerbH,0), oB + glm::vec3(0,kerbH,0), oB,
                 outN, COL_KERB_SIDE);
    }
}

// =============================================================================
// buildFlag
// =============================================================================

void Hole15::buildFlag(const glm::vec3& base, float poleH, float poleR,
                       const glm::vec3& poleColour,
                       const glm::vec3& flagColour)
{
    buildBox(base + glm::vec3(-poleR, 0.0f, -poleR),
             base + glm::vec3( poleR, poleH,  poleR), poleColour);

    glm::vec3 pBase = base + glm::vec3(0, poleH, 0);
    glm::vec3 pTip  = pBase + glm::vec3(0.55f, -0.30f, 0);
    glm::vec3 pBot  = pBase + glm::vec3(0,     -0.30f, 0);

    unsigned int bi = static_cast<unsigned int>(mVertices.size());
    mVertices.push_back({ pBase, {0,0,1}, flagColour });
    mVertices.push_back({ pTip,  {0,0,1}, flagColour });
    mVertices.push_back({ pBot,  {0,0,1}, flagColour });
    mIndices.push_back(bi); mIndices.push_back(bi+1); mIndices.push_back(bi+2);
}

// =============================================================================
// buildCup
// =============================================================================

void Hole15::buildCup(const glm::vec3& centre, const glm::vec3& colour)
{
    float h = CUP_HALF;
    buildBox(centre + glm::vec3(-h, -SLAB_THICK, -h),
             centre + glm::vec3( h,  0.01f,       h), colour);
}

// =============================================================================
// buildTeePatch — dark green rectangle at the tee
// =============================================================================

void Hole15::buildTeePatch(const glm::vec3& centre,
                           const glm::vec3& /*forwardDir*/,
                           float halfW, float depth,
                           const glm::vec3& colour)
{
    glm::vec3 mn = centre + glm::vec3(-depth * 0.5f, -SLAB_THICK * 0.5f, -halfW);
    glm::vec3 mx = centre + glm::vec3( depth * 0.5f,  0.01f,              halfW);
    buildBox(mn, mx, colour);
}

// =============================================================================
// buildEye — decorative eye on the mantle top
// =============================================================================

void Hole15::buildEye(const glm::vec3& centre, float erx, float erz, int segs,
                      const glm::vec3& scleraColour,
                      const glm::vec3& pupilColour)
{
    glm::vec3 nUp { 0,1,0 };
    float ey = 0.005f;

    // Sclera disc
    for (int i = 0; i < segs; ++i)
    {
        float a0 = 2.0f * PI * float(i)     / float(segs);
        float a1 = 2.0f * PI * float(i + 1) / float(segs);
        glm::vec3 vA { centre.x + erx * std::cos(a0), ey, centre.z + erz * std::sin(a0) };
        glm::vec3 vB { centre.x + erx * std::cos(a1), ey, centre.z + erz * std::sin(a1) };
        glm::vec3 cen { centre.x, ey, centre.z };
        unsigned int bi = static_cast<unsigned int>(mVertices.size());
        mVertices.push_back({ cen, nUp, scleraColour });
        mVertices.push_back({ vA,  nUp, scleraColour });
        mVertices.push_back({ vB,  nUp, scleraColour });
        mIndices.push_back(bi); mIndices.push_back(bi+1); mIndices.push_back(bi+2);
    }

    // Pupil disc (smaller)
    float pr = erx * 0.38f;
    for (int i = 0; i < segs; ++i)
    {
        float a0 = 2.0f * PI * float(i)     / float(segs);
        float a1 = 2.0f * PI * float(i + 1) / float(segs);
        glm::vec3 vA { centre.x + pr * std::cos(a0), ey*2.0f, centre.z + pr * std::sin(a0) };
        glm::vec3 vB { centre.x + pr * std::cos(a1), ey*2.0f, centre.z + pr * std::sin(a1) };
        glm::vec3 cen { centre.x, ey*2.0f, centre.z };
        unsigned int bi = static_cast<unsigned int>(mVertices.size());
        mVertices.push_back({ cen, nUp, pupilColour });
        mVertices.push_back({ vA,  nUp, pupilColour });
        mVertices.push_back({ vB,  nUp, pupilColour });
        mIndices.push_back(bi); mIndices.push_back(bi+1); mIndices.push_back(bi+2);
    }
}

// =============================================================================
// buildTentacle / buildTentacleKerb — stubs kept for header compatibility.
// The corridor geometry is built inline in init() below.
// =============================================================================

void Hole15::buildTentacle(const glm::vec3&, const glm::vec3&, const glm::vec3&,
                            float, float, float, float, float, int, const glm::vec3&) {}

void Hole15::buildTentacleKerb(const glm::vec3&, const glm::vec3&, const glm::vec3&,
                                float, float, float, float, int, float, float, const glm::vec3&) {}

// =============================================================================
// init — build all geometry and upload to GPU
// =============================================================================

void Hole15::init(GLuint shaderProgram)
{
    if (mInitialised) return;
    mProgram = shaderProgram;

    mVertices.clear();
    mIndices.clear();

    // =========================================================================
    // 1. MANTLE SLAB (large elliptical head bulge)
    // =========================================================================
    buildMantle(MANTLE_RX, MANTLE_RZ, SLAB_THICK, MANTLE_SEGS,
                COL_TURF, COL_TURF_SIDE);

    // =========================================================================
    // 2. MANTLE PERIMETER KERB (with gaps at the two corridor junctions)
    // =========================================================================
    buildMantleKerb(MANTLE_RX, MANTLE_RZ, MANTLE_SEGS,
                    KERB_H, KERB_T, COL_KERB);

    // =========================================================================
    // 3. RIGHT CORRIDOR — tee entry, extends from +MANTLE_RX in +X direction
    // =========================================================================
    {
        // Corridor slab corners (top face, y = 0)
        // Runs from x = MANTLE_RX  to  x = MANTLE_RX + CORR_LENGTH
        float xNear = MANTLE_RX;
        float xFar  = MANTLE_RX + CORR_LENGTH;
        float hw    = CORR_HALF_W;
        float y0    = -SLAB_THICK;
        float y1    =  0.0f;

        glm::vec3 nUp   { 0,  1, 0 };
        glm::vec3 nDown { 0, -1, 0 };

        // Top
        pushQuad({xNear, y1, -hw}, {xFar, y1, -hw},
                 {xFar,  y1,  hw}, {xNear, y1,  hw},
                 nUp, COL_TURF);
        // Bottom
        pushQuad({xNear, y0, -hw}, {xNear, y0,  hw},
                 {xFar,  y0,  hw}, {xFar,  y0, -hw},
                 nDown, COL_TURF_SIDE);
        // Far end cap (+X face)
        pushQuad({xFar, y0, -hw}, {xFar, y0,  hw},
                 {xFar, y1,  hw}, {xFar, y1, -hw},
                 {1,0,0}, COL_TURF_SIDE);
        // -Z side
        pushQuad({xNear, y1, -hw}, {xNear, y0, -hw},
                 {xFar,  y0, -hw}, {xFar,  y1, -hw},
                 {0,0,-1}, COL_TURF_SIDE);
        // +Z side
        pushQuad({xNear, y0,  hw}, {xNear, y1,  hw},
                 {xFar,  y1,  hw}, {xFar,  y0,  hw},
                 {0,0, 1}, COL_TURF_SIDE);

        // ----- Kerbs along the right corridor -----
        float kH = KERB_H, kT = KERB_T;

        // -Z kerb (top + outer face)
        pushQuad({xNear, y1,    -hw}, {xNear, y1,    -(hw+kT)},
                 {xFar,  y1,    -(hw+kT)}, {xFar, y1,    -hw},
                 nUp, COL_KERB);
        pushQuad({xNear, y1,    -(hw+kT)}, {xNear, y1+kH, -(hw+kT)},
                 {xFar,  y1+kH, -(hw+kT)}, {xFar, y1,    -(hw+kT)},
                 {0,0,-1}, COL_KERB_SIDE);

        // +Z kerb
        pushQuad({xNear, y1,    hw},   {xFar,  y1,    hw},
                 {xFar,  y1,    hw+kT},{xNear, y1,    hw+kT},
                 nUp, COL_KERB);
        pushQuad({xNear, y1,    hw+kT},{xFar,  y1,    hw+kT},
                 {xFar,  y1+kH, hw+kT},{xNear, y1+kH, hw+kT},
                 {0,0, 1}, COL_KERB_SIDE);

        // Far-end kerb cap (+X end)
        pushQuad({xFar, y1, -hw},    {xFar, y1+kH, -hw},
                 {xFar, y1+kH,  hw}, {xFar, y1,     hw},
                 {1,0,0}, COL_KERB_SIDE);

        // ----- Tee patch (dark green box near the far end) -----
        float teeX = xFar - 0.4f;
        buildBox({ teeX - 0.3f, y1, -hw + 0.05f },
                 { teeX + 0.3f, y1 + 0.01f, hw - 0.05f }, COL_TEE);
    }

    // =========================================================================
    // 4. LEFT CORRIDOR — cup exit, extends from -MANTLE_RX in -X direction
    // =========================================================================
    {
        float xNear = -MANTLE_RX;
        float xFar  = -(MANTLE_RX + CORR_LENGTH);
        float hw    = CORR_HALF_W;
        float y0    = -SLAB_THICK;
        float y1    =  0.0f;

        glm::vec3 nUp   { 0,  1, 0 };
        glm::vec3 nDown { 0, -1, 0 };

        // Top
        pushQuad({xNear, y1, -hw}, {xNear, y1,  hw},
                 {xFar,  y1,  hw}, {xFar,  y1, -hw},
                 nUp, COL_TURF);
        // Bottom
        pushQuad({xNear, y0, -hw}, {xFar,  y0, -hw},
                 {xFar,  y0,  hw}, {xNear, y0,  hw},
                 nDown, COL_TURF_SIDE);
        // Far end cap (-X face)
        pushQuad({xFar, y0, -hw}, {xFar, y1, -hw},
                 {xFar, y1,  hw}, {xFar, y0,  hw},
                 {-1,0,0}, COL_TURF_SIDE);
        // -Z side
        pushQuad({xNear, y0, -hw}, {xNear, y1, -hw},
                 {xFar,  y1, -hw}, {xFar,  y0, -hw},
                 {0,0,-1}, COL_TURF_SIDE);
        // +Z side
        pushQuad({xNear, y1,  hw}, {xNear, y0,  hw},
                 {xFar,  y0,  hw}, {xFar,  y1,  hw},
                 {0,0, 1}, COL_TURF_SIDE);

        // ----- Kerbs along the left corridor -----
        float kH = KERB_H, kT = KERB_T;

        // -Z kerb
        pushQuad({xNear, y1, -hw},   {xFar,  y1, -hw},
                 {xFar,  y1, -(hw+kT)}, {xNear, y1, -(hw+kT)},
                 nUp, COL_KERB);
        pushQuad({xNear, y1, -(hw+kT)}, {xNear, y1+kH, -(hw+kT)},
                 {xFar,  y1+kH, -(hw+kT)}, {xFar, y1, -(hw+kT)},
                 {0,0,-1}, COL_KERB_SIDE);

        // +Z kerb
        pushQuad({xNear, y1, hw+kT}, {xFar, y1, hw+kT},
                 {xFar,  y1, hw},    {xNear, y1, hw},
                 nUp, COL_KERB);
        pushQuad({xNear, y1, hw+kT}, {xFar, y1, hw+kT},
                 {xFar,  y1+kH, hw+kT}, {xNear, y1+kH, hw+kT},
                 {0,0, 1}, COL_KERB_SIDE);

        // Far-end kerb cap (-X end)
        pushQuad({xFar, y1,  hw},   {xFar, y1+kH,  hw},
                 {xFar, y1+kH, -hw},{xFar, y1,     -hw},
                 {-1,0,0}, COL_KERB_SIDE);

        // ----- Cup + flag at the far end of left corridor -----
        glm::vec3 cupPos { xFar + 0.35f, 0.0f, 0.0f };
        buildCup(cupPos, COL_CUP);
        buildFlag(cupPos + glm::vec3(0, 0.01f, 0),
                  FLAG_POLE_H, FLAG_POLE_R, COL_POLE, COL_FLAG);
    }

    // =========================================================================
    // 5. UPLOAD TO GPU
    // =========================================================================
    // =========================================================================
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mVertices.size() * sizeof(Vertex)),
                 mVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mIndices.size() * sizeof(unsigned int)),
                 mIndices.data(), GL_STATIC_DRAW);

    // aPos (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(0);

    // aNormal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // aColor (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mIndexCount = static_cast<GLsizei>(mIndices.size());

    mVertices.clear();  mVertices.shrink_to_fit();
    mIndices.clear();   mIndices.shrink_to_fit();

    // =========================================================================
    // 6. CACHE UNIFORM LOCATIONS
    // =========================================================================
    mLocMVP        = glGetUniformLocation(mProgram, "uMVP");
    mLocModel      = glGetUniformLocation(mProgram, "uModel");
    mLocNormalMat  = glGetUniformLocation(mProgram, "uNormalMat");
    mLocLightDir   = glGetUniformLocation(mProgram, "uLightDir");
    mLocLightColor = glGetUniformLocation(mProgram, "uLightColor");
    mLocAmbient    = glGetUniformLocation(mProgram, "uAmbient");
    mLocViewPos    = glGetUniformLocation(mProgram, "uViewPos");

    mInitialised = true;
}

// =============================================================================
// draw
// =============================================================================

void Hole15::draw(const glm::mat4& view,
                   const glm::mat4& proj,
                   const glm::vec3& sunDir,
                   const glm::vec3& camPos)
{
    if (!mInitialised) return;

    glm::mat4 model     = glm::mat4(1.0f);
    glm::mat4 mvp       = proj * view * model;
    glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(model)));

    glUseProgram(mProgram);

    glUniformMatrix4fv(mLocMVP,       1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(mLocModel,     1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(mLocNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));

    glUniform3fv(mLocLightDir,   1, glm::value_ptr(glm::normalize(sunDir)));
    glUniform3f (mLocLightColor, 1.0f, 0.97f, 0.90f);
    glUniform3f (mLocAmbient,    0.22f, 0.22f, 0.25f);
    glUniform3fv(mLocViewPos,    1, glm::value_ptr(camPos));

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// =============================================================================
// cleanup
// =============================================================================

void Hole15::cleanup()
{
    if (!mInitialised) return;

    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
    glDeleteBuffers(1, &mEBO);

    mVAO = mVBO = mEBO = 0;
    mIndexCount  = 0;
    mInitialised = false;
}