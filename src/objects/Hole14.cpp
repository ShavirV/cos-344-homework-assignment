// =============================================================================
// Hole14.cpp
// Implementation of the descending S-curve fairway for Hole 14.
//
// Approach in one paragraph
// -------------------------
// The fairway is a swept ribbon. We pick a centreline curve C(s) for
// s ∈ [0, 1], compute a local frame (tangent, side, up) at evenly spaced
// samples, and emit a quad strip between successive samples for each face
// (top, bottom, left side, right side). The kerbs are extra ribbons offset
// from the fairway edges. To produce the "narrow areas" requested, the
// fairway's left and right half-widths are functions of s — Gaussian bumps
// at specific s values that pinch the corridor inward. The kerb on each
// side has a *constant* outer extent but a varying inner extent (the bumps
// stick into the fairway), so its inner edge bulges where the pinches are.
// Everything else (shader, lighting, textures, dev viewer) is unchanged.
// =============================================================================

#include "Hole14.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

// -----------------------------------------------------------------------------
// Hole-14 geometry constants. Tuned in conjunction with the centreline math
// (see Hole14.h) so that the average descent slope reads as ~13° with steepest
// local slope ~20°, and the S-curve swings 3m to each side.
// -----------------------------------------------------------------------------
namespace
{
    // ── Centreline shape ─────────────────────────────────────────────────────
    const float START_HEIGHT = 4.0f;     // tee height above the cup
    const float Z_LEN        = 11.0f;    // forward extent of the hole
    const float S_AMPLITUDE  = 3.0f;     // X-excursion of the S-curve

    // ── Fairway slab ─────────────────────────────────────────────────────────
    const float BASE_HALF_W  = 3.5f;     // un-pinched half-width
    const float SLAB_THICK   = 0.2f;     // visible thickness of the fairway slab

    // ── Kerbs ────────────────────────────────────────────────────────────────
    const float KERB_HEIGHT     = 0.45f;
    const float KERB_THICKNESS  = 0.35f;

    // ── Sampling resolution ──────────────────────────────────────────────────
    // 40 samples → ~1.7m between segments. Smooth enough that the S-curve
    // doesn't look polygonal at typical viewing distances.
    const int   CENTRELINE_SAMPLES = 40;

    // ── Colours (same swatches as Hole 13 so the shader's material detector
    //    classifies them identically) ──────────────────────────────────────
    const glm::vec3 COLOR_TURF (0.20f, 0.62f, 0.22f);
    const glm::vec3 COLOR_TEE  (0.13f, 0.45f, 0.16f);
    const glm::vec3 COLOR_CUP  (0.04f, 0.04f, 0.04f);
    const glm::vec3 COLOR_KERB (0.72f, 0.71f, 0.69f);
    const glm::vec3 COLOR_POLE (0.95f, 0.95f, 0.95f);
    const glm::vec3 COLOR_FLAG (0.85f, 0.10f, 0.10f);

    // ── Cup + flag ───────────────────────────────────────────────────────────
    const float CUP_RADIUS  = 0.55f;
    const float CUP_DEPTH   = 0.15f;
    const float POLE_HEIGHT = 2.2f;
    const float POLE_RADIUS = 0.06f;

    // ── Tee platform ─────────────────────────────────────────────────────────
    const float TEE_PLATFORM_DEPTH = 2.5f;   // length along the tangent
    const float TEE_PLATFORM_HALFW = 3.5f;   // half-width perpendicular

    // ── Pinches ──────────────────────────────────────────────────────────────
    // Each entry: (s-centre, sigma, magnitude in metres, side)
    //   side > 0 → reduces leftHalfW   (left kerb protrudes inward)
    //   side < 0 → reduces rightHalfW  (right kerb protrudes inward)
    //   side = 0 → reduces both        (gate-style pinch)
    // The pattern alternates left/right along s to give the zigzag corridor.
    struct Pinch { float s; float sigma; float mag; int side; };
    const Pinch PINCHES[] = {
        { 0.18f, 0.04f, 1.2f, +1 },   // left wall in early
        { 0.42f, 0.05f, 1.5f, -1 },   // right wall in middle-early
        { 0.62f, 0.04f, 1.0f, +1 },   // left wall in middle-late
        { 0.82f, 0.05f, 1.4f, -1 },   // right wall in late
    };
    const int NUM_PINCHES = sizeof(PINCHES) / sizeof(PINCHES[0]);

    // ── Hardcoded camera framing for dev.cpp ─────────────────────────────────
    // Centre the framing on the middle of the hole, halfway down the descent.
    // Radius generous enough to cover the whole S-curve + tee height.
    const glm::vec3 SCENE_CENTER(0.0f, 2.0f, 5.5f);
    const float     SCENE_RADIUS = 18.0f;
}

// =============================================================================
// Construction / destruction
// =============================================================================

Hole14::Hole14()
    : mVAO(0), mVBO(0), mEBO(0),
      mIndexCount(0),
      mInitialised(false),
      mProgram(0),
      mLocMVP(-1), mLocModel(-1), mLocNormalMat(-1),
      mLocLightDir(-1), mLocLightColor(-1), mLocAmbient(-1), mLocViewPos(-1),
      mCenter(SCENE_CENTER), mRadius(SCENE_RADIUS)
{
}

Hole14::~Hole14()
{
    cleanup();
}

// =============================================================================
// Centreline + pinch maths
// =============================================================================

glm::vec3 Hole14::centreline(float s) const
{
    // s in [0, 1]; s=0 is the tee end (raised), s=1 is the cup end (ground).
    float x = S_AMPLITUDE * std::sin(2.0f * 3.14159265358979323846f * s);
    float y = START_HEIGHT * (1.0f - s);
    float z = Z_LEN * s;
    return glm::vec3(x, y, z);
}

glm::vec3 Hole14::tangent(float s) const
{
    // Central difference for a stable tangent estimate. eps small enough to
    // be local, large enough that floating-point noise doesn't dominate.
    const float eps = 1e-3f;
    float s1 = std::max(0.0f, s - eps);
    float s2 = std::min(1.0f, s + eps);
    glm::vec3 p1 = centreline(s1);
    glm::vec3 p2 = centreline(s2);
    glm::vec3 t  = p2 - p1;
    float len = glm::length(t);
    if (len < 1e-6f) return glm::vec3(0.0f, 0.0f, 1.0f);  // degenerate guard
    return t / len;
}

void Hole14::pinchProfile(float s, float& leftHalfW, float& rightHalfW) const
{
    leftHalfW  = BASE_HALF_W;
    rightHalfW = BASE_HALF_W;
    for (int i = 0; i < NUM_PINCHES; ++i)
    {
        const Pinch& p = PINCHES[i];
        float d = s - p.s;
        // Standard 1-D Gaussian — symmetric around s = p.s, width controlled
        // by sigma. Max value = mag; falls off smoothly to 0 at distance.
        float bump = p.mag * std::exp(-(d * d) / (2.0f * p.sigma * p.sigma));
        if (p.side >= 0) leftHalfW  -= bump;
        if (p.side <= 0) rightHalfW -= bump;
    }
    // Clamp to a positive minimum so the fairway never closes off (would
    // make degenerate triangles).
    const float minHW = 0.8f;
    if (leftHalfW  < minHW) leftHalfW  = minHW;
    if (rightHalfW < minHW) rightHalfW = minHW;
}

// =============================================================================
// Low-level geometry helpers (identical to Hole13's, kept local to avoid any
// coupling between the two classes — each hole is self-contained).
// =============================================================================

void Hole14::pushQuad(const glm::vec3& a, const glm::vec3& b,
                      const glm::vec3& c, const glm::vec3& d,
                      const glm::vec3& n, const glm::vec3& colour)
{
    unsigned int base = (unsigned int)mVertices.size();
    mVertices.push_back({a, n, colour});
    mVertices.push_back({b, n, colour});
    mVertices.push_back({c, n, colour});
    mVertices.push_back({d, n, colour});
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 1);
    mIndices.push_back(base + 2);
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 2);
    mIndices.push_back(base + 3);
}

void Hole14::buildBox(const glm::vec3& mn, const glm::vec3& mx,
                      const glm::vec3& colour)
{
    glm::vec3 p000(mn.x, mn.y, mn.z), p100(mx.x, mn.y, mn.z);
    glm::vec3 p110(mx.x, mx.y, mn.z), p010(mn.x, mx.y, mn.z);
    glm::vec3 p001(mn.x, mn.y, mx.z), p101(mx.x, mn.y, mx.z);
    glm::vec3 p111(mx.x, mx.y, mx.z), p011(mn.x, mx.y, mx.z);
    pushQuad(p001, p101, p111, p011, { 0, 0,  1}, colour);
    pushQuad(p100, p000, p010, p110, { 0, 0, -1}, colour);
    pushQuad(p000, p001, p011, p010, {-1, 0,  0}, colour);
    pushQuad(p101, p100, p110, p111, { 1, 0,  0}, colour);
    pushQuad(p011, p111, p110, p010, { 0, 1,  0}, colour);
    pushQuad(p000, p100, p101, p001, { 0,-1,  0}, colour);
}

// =============================================================================
// buildFairwayRibbon — sweep a ribbon along the centreline
// =============================================================================
//
// At each sample s_i, we compute:
//   c_i  = centreline(s_i)
//   t_i  = tangent(s_i)
//   side_i = normalize(cross(t_i, worldUp))   // perpendicular to t in the
//                                             // horizontal plane
//   leftHalfW, rightHalfW = pinchProfile(s_i)
//
// The two top edge points are:
//   leftTop_i  = c_i + leftHalfW * side_i      // left of travel direction
//   rightTop_i = c_i - rightHalfW * side_i     // right of travel direction
// And we drop slabThick along *world* -Y (not along the local normal) to get
// the bottom edge points. Using world-Y means the slab thickness is constant
// when viewed from above — the slope of the top surface is real, but the
// bottom is parallel to the top in screen space rather than tilted, which
// reads more like a real fairway than a uniformly thick ramp would.

void Hole14::buildFairwayRibbon(int   sSamples,
                                float slabThick,
                                const glm::vec3& colour)
{
    // Pre-compute every ring's four edge points to avoid recomputing the
    // local frame and pinches multiple times per ring.
    int N = sSamples;
    std::vector<glm::vec3> leftTop (N + 1), rightTop (N + 1);
    std::vector<glm::vec3> leftBot (N + 1), rightBot (N + 1);
    std::vector<glm::vec3> topNorm (N + 1);   // per-ring top normal (for top face)
    std::vector<glm::vec3> sideVec (N + 1);   // per-ring side direction

    for (int i = 0; i <= N; ++i)
    {
        float s = (float)i / (float)N;
        glm::vec3 c = centreline(s);
        glm::vec3 t = tangent(s);
        // Right-handed local frame: side = horizontal-perpendicular to t.
        // worldUp = (0, 1, 0). If t happens to be vertical (it never is here
        // since we're descending at ~15°) the cross would be zero; guard
        // anyway.
        glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
        float sideLen = glm::length(side);
        if (sideLen < 1e-6f)
            side = glm::vec3(1.0f, 0.0f, 0.0f);
        else
            side /= sideLen;

        // Local up = side × tangent. This is the proper "fairway surface
        // normal" — perpendicular to both the direction of travel and the
        // side. It tilts with the slope.
        glm::vec3 up = glm::cross(side, t);
        // up should be near +Y for a descending fairway; sanity-clamp.
        if (up.y < 0.0f) up = -up;

        float leftHW, rightHW;
        pinchProfile(s, leftHW, rightHW);

        // The edge points are offset along `side`. By convention "left"
        // means +side (relative to direction of travel).
        leftTop [i] = c + side * leftHW;
        rightTop[i] = c - side * rightHW;

        // Bottom edges: drop straight down along world -Y by slabThick.
        leftBot [i] = leftTop [i] - glm::vec3(0.0f, slabThick, 0.0f);
        rightBot[i] = rightTop[i] - glm::vec3(0.0f, slabThick, 0.0f);

        topNorm[i] = up;
        sideVec[i] = side;
    }

    // Now stitch quad strips for each face.
    for (int i = 0; i < N; ++i)
    {
        // ── Top face (turf) ──────────────────────────────────────────────────
        // Winding: leftTop[i] → rightTop[i] → rightTop[i+1] → leftTop[i+1]
        // (CCW when viewed from +up — which is the local up direction)
        //
        // Average the two adjacent local-up normals so the strip looks
        // smoothly shaded (Gouraud-style) where the slope changes. Since
        // pushQuad takes a single flat normal we'd lose that — but the
        // S-curve is gentle enough that a single mid-quad normal is fine.
        glm::vec3 nTop = glm::normalize(topNorm[i] + topNorm[i + 1]);
        pushQuad(leftTop[i], rightTop[i], rightTop[i + 1], leftTop[i + 1],
                 nTop, colour);

        // ── Bottom face (-up, reversed winding) ──────────────────────────────
        pushQuad(leftBot[i], leftBot[i + 1], rightBot[i + 1], rightBot[i],
                 -nTop, colour);

        // ── Left side strip ──────────────────────────────────────────────────
        // The outward normal of the left edge is +side (left points to +side).
        glm::vec3 nSide = glm::normalize(sideVec[i] + sideVec[i + 1]);
        // CCW from outside: leftBot[i] → leftBot[i+1] → leftTop[i+1] → leftTop[i]
        pushQuad(leftBot[i], leftBot[i + 1], leftTop[i + 1], leftTop[i],
                 nSide, colour);

        // ── Right side strip ─────────────────────────────────────────────────
        // Right edge faces -side.
        pushQuad(rightBot[i + 1], rightBot[i], rightTop[i], rightTop[i + 1],
                 -nSide, colour);
    }

    // End caps (tee end and cup end of the ribbon) so the slab isn't open.
    // Tee end cap: at i=0, facing -tangent (back toward the tee).
    {
        glm::vec3 t0 = tangent(0.0f);
        pushQuad(rightBot[0], leftBot[0], leftTop[0], rightTop[0],
                 -t0, colour);
    }
    // Cup end cap: at i=N, facing +tangent (forward beyond the cup).
    {
        glm::vec3 tN = tangent(1.0f);
        pushQuad(leftBot[N], rightBot[N], rightTop[N], leftTop[N],
                 tN, colour);
    }
}

// =============================================================================
// buildKerbRibbon — sweep a kerb along one side
// =============================================================================
//
// The kerb's INNER edge tracks the fairway's edge (so where the fairway
// pinches narrow, the kerb's inner face protrudes inward — exactly the
// "walls stick out" effect).
//
// The kerb's OUTER edge sits at a FIXED offset (BASE_HALF_W + KERB_THICKNESS)
// from the centreline. This way the outer perimeter of the hole is a smooth
// continuous curve, and only the inner face wobbles. Visually this reads as
// "the wall is thicker here" rather than "the wall is wonkily placed".
//
// Each kerb is built as a four-faced ribbon: top, outer side, inner side,
// and two end caps. We skip the bottom face (the kerb sits on the fairway
// slab top, hidden from view).

void Hole14::buildKerbRibbon(int   sSamples,
                             float kerbH, float kerbT,
                             float baseHalfW,
                             const glm::vec3& colour,
                             bool  leftSide)
{
    int N = sSamples;
    std::vector<glm::vec3> innerBot(N + 1), outerBot(N + 1);
    std::vector<glm::vec3> innerTop(N + 1), outerTop(N + 1);
    std::vector<glm::vec3> sideVec(N + 1);

    for (int i = 0; i <= N; ++i)
    {
        float s = (float)i / (float)N;
        glm::vec3 c = centreline(s);
        glm::vec3 t = tangent(s);
        glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
        float sLen = glm::length(side);
        if (sLen < 1e-6f) side = glm::vec3(1.0f, 0.0f, 0.0f);
        else              side /= sLen;

        float leftHW, rightHW;
        pinchProfile(s, leftHW, rightHW);

        // Inner edge: hugs the (possibly-pinched) fairway boundary.
        // Outer edge: constant offset, regardless of pinches.
        if (leftSide)
        {
            innerBot[i] = c + side * leftHW;
            outerBot[i] = c + side * (baseHalfW + kerbT);
        }
        else
        {
            innerBot[i] = c - side * rightHW;
            outerBot[i] = c - side * (baseHalfW + kerbT);
        }
        // The kerb sits ON TOP of the fairway slab, so its "bottom" is at the
        // fairway top surface. The top is bottom + kerbH (in world up).
        innerTop[i] = innerBot[i] + glm::vec3(0.0f, kerbH, 0.0f);
        outerTop[i] = outerBot[i] + glm::vec3(0.0f, kerbH, 0.0f);

        sideVec[i] = side;
    }

    // Stitch quad strips. The outward normal for the OUTER side is +side (for
    // the left kerb) or -side (for the right kerb). For the INNER face it's
    // the opposite. We compute that once based on `leftSide`.
    glm::vec3 outDirSign(leftSide ? 1.0f : -1.0f);

    for (int i = 0; i < N; ++i)
    {
        glm::vec3 sideAvg = glm::normalize(sideVec[i] + sideVec[i + 1]);
        glm::vec3 outN = sideAvg * (leftSide ? 1.0f : -1.0f);

        // ── Top face (+Y) ────────────────────────────────────────────────────
        // Winding from above:
        // left kerb: outer→outer+1→inner+1→inner   (CCW from +Y when leftSide=true)
        // right kerb: inner→inner+1→outer+1→outer
        if (leftSide)
        {
            pushQuad(outerTop[i], outerTop[i + 1], innerTop[i + 1], innerTop[i],
                     glm::vec3(0, 1, 0), colour);
        }
        else
        {
            pushQuad(innerTop[i], innerTop[i + 1], outerTop[i + 1], outerTop[i],
                     glm::vec3(0, 1, 0), colour);
        }

        // ── Outer face (faces away from the fairway centreline) ──────────────
        // Walk from bottom outer to top outer along the swept direction.
        if (leftSide)
        {
            pushQuad(outerBot[i], outerBot[i + 1], outerTop[i + 1], outerTop[i],
                     outN, colour);
        }
        else
        {
            // For right kerb, swap winding so the outward normal still leaves
            // the surface front-facing.
            pushQuad(outerBot[i + 1], outerBot[i], outerTop[i], outerTop[i + 1],
                     outN, colour);
        }

        // ── Inner face (faces the fairway centreline) ────────────────────────
        if (leftSide)
        {
            pushQuad(innerBot[i + 1], innerBot[i], innerTop[i], innerTop[i + 1],
                     -outN, colour);
        }
        else
        {
            pushQuad(innerBot[i], innerBot[i + 1], innerTop[i + 1], innerTop[i],
                     -outN, colour);
        }
    }

    // End caps — small quads at the tee and cup ends of the kerb ribbon.
    glm::vec3 t0 = tangent(0.0f), tN = tangent(1.0f);
    if (leftSide)
    {
        // Tee end cap: facing -t0
        pushQuad(innerBot[0], outerBot[0], outerTop[0], innerTop[0],
                 -t0, colour);
        // Cup end cap: facing +tN
        pushQuad(outerBot[N], innerBot[N], innerTop[N], outerTop[N],
                 tN, colour);
    }
    else
    {
        pushQuad(outerBot[0], innerBot[0], innerTop[0], outerTop[0],
                 -t0, colour);
        pushQuad(innerBot[N], outerBot[N], outerTop[N], innerTop[N],
                 tN, colour);
    }
}

// =============================================================================
// Tee platform — a small slab at the start of the descent
// =============================================================================

void Hole14::buildTeePlatform(const glm::vec3& centre,
                              const glm::vec3& tan,
                              float halfW, float depth,
                              float slabThick,
                              const glm::vec3& colour)
{
    // Local frame
    glm::vec3 t = glm::normalize(glm::vec3(tan.x, 0.0f, tan.z));   // horizontal
    if (glm::length(t) < 1e-6f) t = glm::vec3(0, 0, 1);
    glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(side) < 1e-6f) side = glm::vec3(1, 0, 0);
    else side = glm::normalize(side);

    // The platform extends `depth` *backward* from the centre (away from the
    // descent), and `halfW` to each side. It sits at the centre's height,
    // with slabThick thickness downward.
    glm::vec3 back = centre - t * depth;

    glm::vec3 frontL = centre + side * halfW;
    glm::vec3 frontR = centre - side * halfW;
    glm::vec3 backL  = back   + side * halfW;
    glm::vec3 backR  = back   - side * halfW;

    glm::vec3 dn(0.0f, slabThick, 0.0f);
    glm::vec3 frontLb = frontL - dn, frontRb = frontR - dn;
    glm::vec3 backLb  = backL  - dn, backRb  = backR  - dn;

    // Top
    pushQuad(backL, backR, frontR, frontL, glm::vec3(0, 1, 0), colour);
    // Bottom
    pushQuad(backLb, frontLb, frontRb, backRb, glm::vec3(0,-1, 0), colour);
    // Left side (+side direction)
    pushQuad(backLb, backL, frontL, frontLb, side, colour);
    // Right side (-side direction)
    pushQuad(frontRb, frontR, backR, backRb, -side, colour);
    // Back end (away from descent, normal -t)
    pushQuad(backRb, backR, backL, backLb, -t, colour);
    // Front end (where the platform meets the fairway, normal +t)
    pushQuad(frontLb, frontL, frontR, frontRb, t, colour);
}

// =============================================================================
// End wall — closing boundary wall at the cup end of the ribbon
// =============================================================================
//
// A short upright slab that caps the open end of the fairway + kerbs at s=1
// so the hole reads as enclosed rather than cut off in mid-air. The wall
// spans the full kerb-to-kerb width and matches the kerb height.

void Hole14::buildEndWall(const glm::vec3& centre,
                          const glm::vec3& segTan,
                          float halfW,
                          float kerbT,
                          float wallT,
                          float wallH)
{
    // Horizontal side vector (same derivation as in buildKerbRibbon).
    glm::vec3 t    = glm::normalize(segTan);
    glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
    float sLen = glm::length(side);
    if (sLen < 1e-6f) side = glm::vec3(1.0f, 0.0f, 0.0f);
    else              side /= sLen;

    float span  = halfW + kerbT;   // half-span including kerbs
    float baseY = centre.y;        // fairway surface height at s=1
    float topY  = baseY + wallH;

    // Near edge (at the cup end of the ribbon) and far edge (wallT beyond).
    glm::vec3 BL  = centre + span * side;
    glm::vec3 BR  = centre - span * side;
    glm::vec3 BLf = BL + wallT * t;
    glm::vec3 BRf = BR + wallT * t;

    glm::vec3 TL  (BL.x,  topY, BL.z);
    glm::vec3 TR  (BR.x,  topY, BR.z);
    glm::vec3 TLf (BLf.x, topY, BLf.z);
    glm::vec3 TRf (BRf.x, topY, BRf.z);

    // Inner face (faces back up the fairway, normal = -t)
    pushQuad(BR, BL, TL, TR, -t, COLOR_KERB);
    // Outer face (back of wall, normal = +t)
    pushQuad(BLf, BRf, TRf, TLf, t, COLOR_KERB);
    // Top face
    pushQuad(TL, TR, TRf, TLf, glm::vec3(0, 1, 0), COLOR_KERB);
    // Left side (+side)
    pushQuad(BLf, BL, TL, TLf, side, COLOR_KERB);
    // Right side (-side)
    pushQuad(BR, BRf, TRf, TR, -side, COLOR_KERB);
    // Bottom (hidden)
    pushQuad(BL, BLf, BRf, BR, glm::vec3(0, -1, 0), COLOR_KERB);
}

// =============================================================================
// Cup + flag
// =============================================================================

void Hole14::buildCup(const glm::vec3& centre, const glm::vec3& colour)
{
    glm::vec3 mn(centre.x - CUP_RADIUS, centre.y - CUP_DEPTH, centre.z - CUP_RADIUS);
    glm::vec3 mx(centre.x + CUP_RADIUS, centre.y + 0.002f,    centre.z + CUP_RADIUS);
    buildBox(mn, mx, colour);
}

void Hole14::buildFlag(const glm::vec3& base, float poleH, float poleR,
                       const glm::vec3& poleColour,
                       const glm::vec3& flagColour)
{
    // Pole
    glm::vec3 mn(base.x - poleR, base.y,         base.z - poleR);
    glm::vec3 mx(base.x + poleR, base.y + poleH, base.z + poleR);
    buildBox(mn, mx, poleColour);

    // Triangular pennant in +X, double-sided.
    float flagW = 0.9f;
    float flagH = 0.55f;
    glm::vec3 fA(base.x,         base.y + poleH - flagH,        base.z);
    glm::vec3 fB(base.x + flagW, base.y + poleH - flagH * 0.5f, base.z);
    glm::vec3 fC(base.x,         base.y + poleH,                base.z);

    {
        unsigned int idx = (unsigned int)mVertices.size();
        mVertices.push_back({fA, { 0, 0,  1}, flagColour});
        mVertices.push_back({fB, { 0, 0,  1}, flagColour});
        mVertices.push_back({fC, { 0, 0,  1}, flagColour});
        mIndices.push_back(idx + 0);
        mIndices.push_back(idx + 1);
        mIndices.push_back(idx + 2);
    }
    {
        unsigned int idx = (unsigned int)mVertices.size();
        mVertices.push_back({fA, { 0, 0, -1}, flagColour});
        mVertices.push_back({fC, { 0, 0, -1}, flagColour});
        mVertices.push_back({fB, { 0, 0, -1}, flagColour});
        mIndices.push_back(idx + 0);
        mIndices.push_back(idx + 1);
        mIndices.push_back(idx + 2);
    }
}

// =============================================================================
// init — build all CPU-side geometry and upload to GL
// =============================================================================

void Hole14::init(GLuint shaderProgram)
{
    if (mInitialised) return;

    mProgram = shaderProgram;

    // Cache uniform locations — uMVP / uLightColor / uAmbient have moved
    // out of per-hole control (dev.cpp owns them now). See Hole13.cpp for
    // the full explanation.
    mLocModel      = glGetUniformLocation(mProgram, "uModel");
    mLocNormalMat  = glGetUniformLocation(mProgram, "uNormalMat");
    mLocLightDir   = glGetUniformLocation(mProgram, "uLightDir");
    mLocViewPos    = glGetUniformLocation(mProgram, "uViewPos");

    mVertices.clear();
    mIndices.clear();

    // ── 1. Fairway ribbon (turf, swept along the S-curve, pinches baked in) ─
    buildFairwayRibbon(CENTRELINE_SAMPLES, SLAB_THICK, COLOR_TURF);

    // ── 2. Kerb ribbons (left + right) ─────────────────────────────────────
    buildKerbRibbon(CENTRELINE_SAMPLES, KERB_HEIGHT, KERB_THICKNESS,
                    BASE_HALF_W, COLOR_KERB, /*leftSide=*/true);
    buildKerbRibbon(CENTRELINE_SAMPLES, KERB_HEIGHT, KERB_THICKNESS,
                    BASE_HALF_W, COLOR_KERB, /*leftSide=*/false);

    // ── 3. Tee platform at s=0 ──────────────────────────────────────────────
    {
        glm::vec3 teeCentre = centreline(0.0f);
        glm::vec3 teeTan    = tangent(0.0f);
        buildTeePlatform(teeCentre, teeTan,
                         TEE_PLATFORM_HALFW, TEE_PLATFORM_DEPTH,
                         SLAB_THICK, COLOR_TURF);

        // Darker tee patch on top of the platform.
        const float eps = 0.001f;
        glm::vec3 mn(teeCentre.x - 1.2f, teeCentre.y + eps,         teeCentre.z - 1.2f);
        glm::vec3 mx(teeCentre.x + 1.2f, teeCentre.y + 0.02f + eps, teeCentre.z + 1.2f);
        buildBox(mn, mx, COLOR_TEE);
    }

    // ── 4. Cup + flag at s=1 ────────────────────────────────────────────────
    // Pull the cup inward (away from the end wall) so it doesn't clip through.
    // The wall's inner face sits at centreline(1.0); offset by CUP_RADIUS + a
    // small clearance gap back along -tangent (toward the tee).
    {
        glm::vec3 cupEnd = centreline(1.0f);
        glm::vec3 cupTan = tangent(1.0f);
        const float CUP_WALL_CLEARANCE = 0.3f;
        glm::vec3 cupCentre = cupEnd - (CUP_RADIUS + CUP_WALL_CLEARANCE) * cupTan;

        buildCup(cupCentre, COLOR_CUP);
        glm::vec3 poleBase(cupCentre.x, cupCentre.y, cupCentre.z);
        buildFlag(poleBase, POLE_HEIGHT, POLE_RADIUS,
                  COLOR_POLE, COLOR_FLAG);
    }

    // ── 5. Closing boundary wall at the cup end ──────────────────────────────
    // The wall spans the full kerb-to-kerb width at s=1, capping the open end
    // of the fairway so the flag doesn't float at the boundary of nothing.
    {
        glm::vec3 cupCentre = centreline(1.0f);
        glm::vec3 cupTan    = tangent(1.0f);
        buildEndWall(cupCentre,
                     cupTan,
                     BASE_HALF_W,
                     KERB_THICKNESS,
                     /*wallT=*/KERB_THICKNESS,
                     /*wallH=*/KERB_HEIGHT);
    }

    // ── 6. Upload to GPU ────────────────────────────────────────────────────
    if (mVertices.empty() || mIndices.empty())
    {
        std::cerr << "Hole14::init — no geometry was generated!" << std::endl;
        return;
    }

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 mVertices.size() * sizeof(Vertex),
                 mVertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mIndices.size() * sizeof(unsigned int),
                 mIndices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mIndexCount = (GLsizei)mIndices.size();

    mCenter = SCENE_CENTER;
    mRadius = SCENE_RADIUS;

    mVertices.clear(); mVertices.shrink_to_fit();
    mIndices.clear();  mIndices.shrink_to_fit();

    mInitialised = true;
}

// =============================================================================
// draw — same uniform-pushing path as Hole13 (the shader is shared)
// =============================================================================

void Hole14::draw(const glm::mat4& view,
                  const glm::mat4& proj,
                  const glm::vec3& sunDir,
                  const glm::vec3& camPos)
{
    if (!mInitialised) return;

    glUseProgram(mProgram);

glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(150.0f, 0.35f, -15.0f));
    glm::mat3 nrmM  = glm::mat3(glm::transpose(glm::inverse(model)));

    if (mLocModel      != -1) glUniformMatrix4fv(mLocModel,     1, GL_FALSE, glm::value_ptr(model));
    if (mLocNormalMat  != -1) glUniformMatrix3fv(mLocNormalMat, 1, GL_FALSE, glm::value_ptr(nrmM));
    if (mLocLightDir   != -1) glUniform3fv(mLocLightDir,        1, glm::value_ptr(sunDir));
    if (mLocViewPos    != -1) glUniform3fv(mLocViewPos,         1, glm::value_ptr(camPos));

    // view + proj are program-wide; suppress unused-param warnings.
    (void)view; (void)proj;

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Hole14::drawDepth(GLuint depthShader)
{
    if (!mInitialised) return;

    GLint loc_uModel = glGetUniformLocation(depthShader, "uModel");
    if (loc_uModel != -1) {
        glm::mat4 model(1.0f);
        glUniformMatrix4fv(loc_uModel, 1, GL_FALSE, glm::value_ptr(model));
    }

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// =============================================================================
// cleanup
// =============================================================================

void Hole14::cleanup()
{
    if (mEBO) { glDeleteBuffers(1, &mEBO); mEBO = 0; }
    if (mVBO) { glDeleteBuffers(1, &mVBO); mVBO = 0; }
    if (mVAO) { glDeleteVertexArrays(1, &mVAO); mVAO = 0; }
    mIndexCount = 0;
    mInitialised = false;
}