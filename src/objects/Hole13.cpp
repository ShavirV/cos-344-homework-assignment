// =============================================================================
// Hole13.cpp
// Implementation of the boomerang-shaped fairway for Hole 13.
//
// Shape
// -----
// A dogleg with a single 120° bend at the halfway point. Two equal-length
// straight fairway segments meet at the bend; the tee is at one end, the cup
// (with flag) is at the other. Concrete kerbs run along the outer edges of
// each segment. The bend is mitred — the inner and outer edges of the two
// segments meet at single points along the angle bisector, so there is no
// visible seam at the corner.
//
// Layout (looking down at the XZ plane, +X is right, +Z is forward):
//
//          tee (+X, +Z)                          cup (-X, +Z)
//             *                                       *
//              \                                     /
//               \   segment A           segment B   /
//                \                                 /
//                 \                               /
//                  \                             /
//                   \____________ bend _________/        <- 120° interior angle
//                              (origin)
//
// The V opens forward (toward +Z), with the bend at the origin closer to
// the camera. The convex (outer) side of the bend is at -Z; the concave
// (inner) side, between the two segments, is at +Z.
//
// All geometry is in local space around the origin. No ground slab, no AABB
// tracking — sceneCenter() / sceneRadius() return hardcoded values.
// =============================================================================

#include "Hole13.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

// -----------------------------------------------------------------------------
// Constants for this hole's geometry.
// -----------------------------------------------------------------------------
namespace
{
    // ── Dogleg layout ────────────────────────────────────────────────────────
    // Interior angle at the bend. 120° gives a gentle, readable boomerang.
    const float BEND_INTERIOR_DEG = 120.0f;

    // Length of each segment, from the bend to its outer end.
    const float SEGMENT_LEN  = 18.0f;
    // Half-width of the fairway strip (full width = 7m).
    const float FAIRWAY_HALF =  3.5f;

    // ── Bend fillet ──────────────────────────────────────────────────────────
    // Radius of the circular arc that replaces the sharp corner at the bend.
    // The arc is tangent to both straight segments. Setting this to 0 would
    // give back the original mitred corner.
    const float BEND_FILLET_RADIUS = 6.0f;
    // How many quads to use along the fillet arc. ~12 gives a smooth read at
    // typical viewing distances.
    const int   ARC_SEGMENTS_FILLET = 12;

    // Slab geometry (the fairway is a thin slab; ball rolls on top).
    const float FAIRWAY_TOP_Y = 0.2f;
    const float FAIRWAY_BOT_Y = 0.0f;

    // ── Kerbs (concrete fairway borders, per report §6.3) ────────────────────
    const float KERB_HEIGHT     = 0.45f;
    const float KERB_THICKNESS  = 0.35f;

    // ── Colours (linear RGB, roughly matching report §6) ─────────────────────
    const glm::vec3 COLOR_TURF (0.20f, 0.62f, 0.22f);  // bright green
    const glm::vec3 COLOR_TEE  (0.13f, 0.45f, 0.16f);  // darker patch
    const glm::vec3 COLOR_CUP  (0.04f, 0.04f, 0.04f);  // near-black hole
    const glm::vec3 COLOR_KERB (0.72f, 0.71f, 0.69f);  // light grey concrete
    const glm::vec3 COLOR_POLE (0.95f, 0.95f, 0.95f);  // white pole
    const glm::vec3 COLOR_FLAG (0.85f, 0.10f, 0.10f);  // red flag

    // ── Tee marker (small darker patch on the fairway at the start) ──────────
    const float TEE_HALF_SIZE = 1.4f;

    // ── Cup (the actual "hole" the ball goes into) ───────────────────────────
    const float CUP_RADIUS = 0.55f;
    const float CUP_DEPTH  = 0.15f;

    // ── Flag ─────────────────────────────────────────────────────────────────
    const float POLE_HEIGHT = 2.2f;
    const float POLE_RADIUS = 0.06f;

    // ── Hardcoded camera framing for dev.cpp ─────────────────────────────────
    const glm::vec3 SCENE_CENTER(0.0f, 0.0f, 5.0f);
    const float     SCENE_RADIUS = 25.0f;
}

// =============================================================================
// Construction / destruction
// =============================================================================

Hole13::Hole13()
    : mVAO(0), mVBO(0), mEBO(0),
      mIndexCount(0),
      mInitialised(false),
      mProgram(0),
      mLocMVP(-1), mLocModel(-1), mLocNormalMat(-1),
      mLocLightDir(-1), mLocLightColor(-1), mLocAmbient(-1), mLocViewPos(-1),
      mCenter(SCENE_CENTER), mRadius(SCENE_RADIUS)
{
}

Hole13::~Hole13()
{
    cleanup();
}

// =============================================================================
// Low-level geometry helpers
// =============================================================================

void Hole13::pushQuad(const glm::vec3& a, const glm::vec3& b,
                      const glm::vec3& c, const glm::vec3& d,
                      const glm::vec3& n, const glm::vec3& colour)
{
    // a-b-c-d expected CCW when viewed from the +n side.
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

void Hole13::buildBox(const glm::vec3& mn, const glm::vec3& mx,
                      const glm::vec3& colour)
{
    glm::vec3 p000(mn.x, mn.y, mn.z);
    glm::vec3 p100(mx.x, mn.y, mn.z);
    glm::vec3 p110(mx.x, mx.y, mn.z);
    glm::vec3 p010(mn.x, mx.y, mn.z);
    glm::vec3 p001(mn.x, mn.y, mx.z);
    glm::vec3 p101(mx.x, mn.y, mx.z);
    glm::vec3 p111(mx.x, mx.y, mx.z);
    glm::vec3 p011(mn.x, mx.y, mx.z);

    pushQuad(p001, p101, p111, p011, { 0, 0,  1}, colour); // +Z front
    pushQuad(p100, p000, p010, p110, { 0, 0, -1}, colour); // -Z back
    pushQuad(p000, p001, p011, p010, {-1, 0,  0}, colour); // -X left
    pushQuad(p101, p100, p110, p111, { 1, 0,  0}, colour); // +X right
    pushQuad(p011, p111, p110, p010, { 0, 1,  0}, colour); // +Y top
    pushQuad(p000, p100, p101, p001, { 0,-1,  0}, colour); // -Y bottom
}

// =============================================================================
// Fairway slab — built as a flat polygon with mitred corners at the bend.
// =============================================================================
//
// The fairway top face is a hexagonal slab:
//
//     tOuterTee ────────── outerMitre ────────── tOuterCup
//       |                       |                     |
//       |   segment A           |       segment B     |
//       |                       |                     |
//     tInnerTee ────────── innerMitre ────────── tInnerCup
//
// Each segment contributes one quad to the top, one to the bottom, and one to
// each side; the inner edges merge at innerMitre, the outer edges merge at
// outerMitre. This means no seam at the bend, regardless of bend angle.
//
// We compute the four end-point edge vertices (outer/inner × tee/cup) from
// each segment's direction and perpendicular; the mitre points sit on the
// angle bisector through the bend.

static glm::vec3 perpXZ(const glm::vec3& dir)
{
    // Right-hand perpendicular in the XZ plane (rotate 90° clockwise looking
    // down +Y, so +X dir → -Z perp). Pick whichever orientation labels the
    // "outer" side — we'll be consistent throughout this file.
    //
    // For our V-shape opening toward +Z, segment A's direction is
    // (+sin θ, 0, +cos θ) and we want "outer" to be the side away from the
    // bisector — that's the +X side, perpendicular = (cos θ, 0, -sin θ).
    return glm::vec3(dir.z, 0.0f, -dir.x);
}

void Hole13::buildFairwaySlab()
{
    // ── Geometry parameters ──────────────────────────────────────────────────
    // α = half of the interior bend angle. For 120°, α = 60°.
    // The arc fillet has radius R, tangent to both straight segments.
    // From standard analytic geometry of corner fillets:
    //
    //   T = R / tan(α)          — distance from the bend to each tangent point
    //                             along the corresponding straight segment.
    //   C = R / sin(α)          — distance from the bend to the arc centre,
    //                             measured along the inner-angle bisector
    //                             (the +Z bisector in our layout).
    //
    // The arc sweeps the exterior turn angle, which for a 120° interior is 60°.

    float halfInterior = glm::radians(BEND_INTERIOR_DEG * 0.5f);
    float R = BEND_FILLET_RADIUS;
    float T = R / std::tan(halfInterior);
    float C = R / std::sin(halfInterior);

    // Segment directions (from bend outward toward each end).
    glm::vec3 dirA( std::sin(halfInterior), 0.0f, std::cos(halfInterior)); // toward tee
    glm::vec3 dirB(-std::sin(halfInterior), 0.0f, std::cos(halfInterior)); // toward cup

    // Outer-edge perpendiculars (away from the inside of the V).
    glm::vec3 perpA = perpXZ(dirA);   // points to segment A's outer side
    glm::vec3 perpB = -perpXZ(dirB);  // perpXZ(dirB) points the wrong way for B; flip.

    // Bend and arc centre.
    glm::vec3 bend(0.0f, FAIRWAY_TOP_Y, 0.0f);
    glm::vec3 arcCentre(0.0f, FAIRWAY_TOP_Y, C);   // on the inner (+Z) bisector

    // ── Tangent points (where each straight meets the arc) ──────────────────
    // tanA sits T metres along dirA from the bend; tanB likewise along dirB.
    glm::vec3 tanA = bend + T * dirA;
    glm::vec3 tanB = bend + T * dirB;

    // Outer / inner edge points at each tangent point.
    glm::vec3 tanA_outer = tanA + FAIRWAY_HALF * perpA;
    glm::vec3 tanA_inner = tanA - FAIRWAY_HALF * perpA;
    glm::vec3 tanB_outer = tanB + FAIRWAY_HALF * perpB;
    glm::vec3 tanB_inner = tanB - FAIRWAY_HALF * perpB;

    // ── Segment endpoints (tee / cup) ───────────────────────────────────────
    glm::vec3 endA = bend + SEGMENT_LEN * dirA;
    glm::vec3 endB = bend + SEGMENT_LEN * dirB;

    glm::vec3 aOuterEnd = endA + FAIRWAY_HALF * perpA;
    glm::vec3 aInnerEnd = endA - FAIRWAY_HALF * perpA;
    glm::vec3 bOuterEnd = endB + FAIRWAY_HALF * perpB;
    glm::vec3 bInnerEnd = endB - FAIRWAY_HALF * perpB;

    // ── Helper to drop a point to the slab bottom ───────────────────────────
    auto down = [](glm::vec3 v) { v.y = FAIRWAY_BOT_Y; return v; };

    // =========================================================================
    // STRAIGHT PORTION OF SEGMENT A (from tee end to tanA)
    // =========================================================================
    // Top
    pushQuad(aInnerEnd, aOuterEnd, tanA_outer, tanA_inner,
             glm::vec3(0, 1, 0), COLOR_TURF);
    // Bottom (reversed winding)
    pushQuad(down(aInnerEnd), down(tanA_inner), down(tanA_outer), down(aOuterEnd),
             glm::vec3(0, -1, 0), COLOR_TURF);
    // Outer side
    pushQuad(down(aOuterEnd), down(tanA_outer), tanA_outer, aOuterEnd,
             perpA, COLOR_TURF);
    // Inner side
    pushQuad(down(tanA_inner), down(aInnerEnd), aInnerEnd, tanA_inner,
             -perpA, COLOR_TURF);
    // Tee end cap
    pushQuad(down(aInnerEnd), down(aOuterEnd), aOuterEnd, aInnerEnd,
             dirA, COLOR_TURF);

    // =========================================================================
    // STRAIGHT PORTION OF SEGMENT B (from tanB to cup end)
    // =========================================================================
    // Top
    pushQuad(tanB_inner, tanB_outer, bOuterEnd, bInnerEnd,
             glm::vec3(0, 1, 0), COLOR_TURF);
    // Bottom
    pushQuad(down(tanB_inner), down(bInnerEnd), down(bOuterEnd), down(tanB_outer),
             glm::vec3(0, -1, 0), COLOR_TURF);
    // Outer side
    pushQuad(down(tanB_outer), down(bOuterEnd), bOuterEnd, tanB_outer,
             perpB, COLOR_TURF);
    // Inner side
    pushQuad(down(bInnerEnd), down(tanB_inner), tanB_inner, bInnerEnd,
             -perpB, COLOR_TURF);
    // Cup end cap
    pushQuad(down(bOuterEnd), down(bInnerEnd), bInnerEnd, bOuterEnd,
             dirB, COLOR_TURF);

    // =========================================================================
    // ARC FILLET at the corner
    // =========================================================================
    // The fairway centreline arcs around `arcCentre` from tanA to tanB. We
    // tessellate that arc into ARC_SEGMENTS_FILLET steps. At each step we
    // compute the inner-edge point (radius R - HW from arcCentre) and the
    // outer-edge point (radius R + HW). The "outer" edge of the fairway here
    // means the convex/longer arc — that's the LARGER radius from arcCentre,
    // because the arc centre sits on the INSIDE of the V.
    //
    // The starting angle (as seen from arcCentre) is the angle to tanA. By the
    // setup, this is -60° from the +X axis in the XZ plane (atan2(-(C - 0), T)
    // for tanA at (T*sin α, 0, T*cos α) = (R cos α / sin α · sin α, _,
    // R cos α / sin α · cos α) — though it's easier to just compute it).
    //
    // The ending angle (to tanB) is -120°. So we sweep clockwise (decreasing
    // angle) through 60°.
    //
    // Note on edge radii:
    //   outerEdgeR = R + HW   (the wider arc, on the -Z side from arcCentre)
    //   innerEdgeR = R - HW   (the tighter arc, closer to arcCentre)
    // With R = 6, HW = 3.5: outerEdgeR = 9.5, innerEdgeR = 2.5. Fine.

    auto angleFromCentre = [&](const glm::vec3& p) {
        return std::atan2(p.z - arcCentre.z, p.x - arcCentre.x);
    };
    float angTanA = angleFromCentre(tanA);  // ≈ -60° in radians
    float angTanB = angleFromCentre(tanB);  // ≈ -120°
    // We want a continuous sweep from angTanA to angTanB going CLOCKWISE (in the
    // XZ plane, with X right and Z up, clockwise = decreasing atan2).
    // For 120° interior, angTanA > angTanB already, so we just iterate.

    float outerR = R + FAIRWAY_HALF;
    float innerR = R - FAIRWAY_HALF;

    // Pre-compute each ring point so we can stitch quads between successive rings.
    int N = ARC_SEGMENTS_FILLET;
    std::vector<glm::vec3> outerTop(N + 1), innerTop(N + 1);
    std::vector<glm::vec3> outerBot(N + 1), innerBot(N + 1);
    for (int i = 0; i <= N; ++i)
    {
        float t = (float)i / (float)N;
        float ang = angTanA + (angTanB - angTanA) * t;   // interpolate
        float cx = std::cos(ang), sz = std::sin(ang);

        outerTop[i] = glm::vec3(arcCentre.x + outerR * cx,
                                FAIRWAY_TOP_Y,
                                arcCentre.z + outerR * sz);
        innerTop[i] = glm::vec3(arcCentre.x + innerR * cx,
                                FAIRWAY_TOP_Y,
                                arcCentre.z + innerR * sz);
        outerBot[i] = down(outerTop[i]);
        innerBot[i] = down(innerTop[i]);
    }

    // The first ring (i=0) should align exactly with the straight-segment-A
    // tangent edge points; the last ring (i=N) with segment B's. Snap them
    // explicitly to avoid any floating-point seam.
    outerTop[0] = tanA_outer;   innerTop[0] = tanA_inner;
    outerBot[0] = down(tanA_outer); innerBot[0] = down(tanA_inner);
    outerTop[N] = tanB_outer;   innerTop[N] = tanB_inner;
    outerBot[N] = down(tanB_outer); innerBot[N] = down(tanB_inner);

    for (int i = 0; i < N; ++i)
    {
        // Top quad: innerTop[i] → outerTop[i] → outerTop[i+1] → innerTop[i+1]
        // (CCW when viewed from +Y, so the +Y normal is correct)
        pushQuad(innerTop[i], outerTop[i], outerTop[i+1], innerTop[i+1],
                 glm::vec3(0, 1, 0), COLOR_TURF);

        // Bottom (reverse winding for -Y normal)
        pushQuad(innerBot[i], innerBot[i+1], outerBot[i+1], outerBot[i],
                 glm::vec3(0, -1, 0), COLOR_TURF);

        // Outer side strip — outward-pointing normal at this quad is the
        // *radial outward* direction from the arc centre at the midpoint angle.
        float tMid = ((float)i + 0.5f) / (float)N;
        float angMid = angTanA + (angTanB - angTanA) * tMid;
        glm::vec3 radialOut(std::cos(angMid), 0.0f, std::sin(angMid));
        // CCW from outside (positive radial direction):
        // outerBot[i] → outerBot[i+1] → outerTop[i+1] → outerTop[i]
        pushQuad(outerBot[i], outerBot[i+1], outerTop[i+1], outerTop[i],
                 radialOut, COLOR_TURF);

        // Inner side strip — inward-pointing normal is -radialOut.
        // Walk in the reverse order so the inward normal is correct.
        pushQuad(innerBot[i+1], innerBot[i], innerTop[i], innerTop[i+1],
                 -radialOut, COLOR_TURF);
    }

    // ── Bookkeeping for tee/cup placement ───────────────────────────────────
    mTeeEndCentre = glm::vec3(endA.x, FAIRWAY_TOP_Y, endA.z);
    mCupEndCentre = glm::vec3(endB.x, FAIRWAY_TOP_Y, endB.z);
}

// =============================================================================
// Kerbs along both edges of each segment.
// =============================================================================
//
// Each segment gets one outer kerb (on the convex side of the V) and one inner
// kerb (on the concave side). The report describes concrete border walls on
// the perimeter of each hole — keeping the ball in play. The kerbs sit on top
// of the fairway slab. End caps are drawn so they don't look hollow at the
// tee, cup, or where the straight meets the arc.
//
// The `near` end of each kerb is the segment-side end (toward the bend / arc).
// In the pre-fillet version this was the bend; now it's the tangent point.
// Pass it in via `startCentre` so the caller controls where the straight ends.

void Hole13::buildKerbsForSegment(const glm::vec3& startCentre,
                                  const glm::vec3& segDir,
                                  const glm::vec3& outerPerp,
                                  float segLen,
                                  float halfW,
                                  float kerbH, float kerbT)
{
    // Helper: build a single straight kerb as a box.
    auto buildKerb = [&](const glm::vec3& nearOuter, const glm::vec3& nearInner,
                         const glm::vec3& farOuter,  const glm::vec3& farInner,
                         const glm::vec3& outNormal)
    {
        float topY = FAIRWAY_TOP_Y + kerbH;
        glm::vec3 nob = nearOuter;
        glm::vec3 nib = nearInner;
        glm::vec3 fob = farOuter;
        glm::vec3 fib = farInner;
        glm::vec3 not_(nob.x, topY, nob.z);
        glm::vec3 nit(nib.x, topY, nib.z);
        glm::vec3 fot(fob.x, topY, fob.z);
        glm::vec3 fit(fib.x, topY, fib.z);

        // Top
        pushQuad(nit, not_, fot, fit, glm::vec3(0, 1, 0), COLOR_KERB);
        // Bottom
        pushQuad(nib, fib, fob, nob, glm::vec3(0,-1, 0), COLOR_KERB);
        // Outer side
        pushQuad(nob, fob, fot, not_, outNormal, COLOR_KERB);
        // Inner side
        pushQuad(fib, nib, nit, fit, -outNormal, COLOR_KERB);
        // Near end cap
        glm::vec3 nearN = glm::normalize(nob - fob);
        pushQuad(nib, nob, not_, nit, nearN, COLOR_KERB);
        // Far end cap
        glm::vec3 farN = -nearN;
        pushQuad(fob, fib, fit, fot, farN, COLOR_KERB);
    };

    // Outer kerb (sits outside the fairway outer edge).
    {
        glm::vec3 kerbInnerNear = startCentre + halfW * outerPerp;
        glm::vec3 kerbInnerFar  = kerbInnerNear + segLen * segDir;
        glm::vec3 kerbOuterNear = startCentre + (halfW + kerbT) * outerPerp;
        glm::vec3 kerbOuterFar  = kerbOuterNear + segLen * segDir;
        buildKerb(kerbOuterNear, kerbInnerNear,
                  kerbOuterFar,  kerbInnerFar,
                  outerPerp);
    }

    // Inner kerb (sits outside the fairway inner edge, on the concave side).
    {
        glm::vec3 innerPerp = -outerPerp;
        glm::vec3 kerbInnerNear = startCentre + halfW * innerPerp;
        glm::vec3 kerbInnerFar  = kerbInnerNear + segLen * segDir;
        glm::vec3 kerbOuterNear = startCentre + (halfW + kerbT) * innerPerp;
        glm::vec3 kerbOuterFar  = kerbOuterNear + segLen * segDir;
        buildKerb(kerbOuterNear, kerbInnerNear,
                  kerbOuterFar,  kerbInnerFar,
                  innerPerp);
    }
}

// =============================================================================
// Arc kerbs along the fillet at the bend.
// =============================================================================
//
// Wraps the two fairway edges of the arc with thin kerbs of height `kerbH`
// and thickness `kerbT`. The outer kerb sits at radii (R+HW) → (R+HW+kerbT),
// the inner kerb at radii (R-HW-kerbT) → (R-HW). Tessellated to match the
// fairway arc so the seams line up.

void Hole13::buildArcKerbs(const glm::vec3& arcCentre,
                           float arcR,
                           float halfW,
                           float angStart, float angEnd,
                           int   segments,
                           float kerbH, float kerbT)
{
    auto buildRing = [&](float rInner, float rOuter, bool outwardIsLargerR)
    {
        float topY  = FAIRWAY_TOP_Y + kerbH;
        float botY  = FAIRWAY_TOP_Y;

        // Pre-compute ring vertices.
        int N = segments;
        std::vector<glm::vec3> outerTop(N + 1), innerTop(N + 1);
        std::vector<glm::vec3> outerBot(N + 1), innerBot(N + 1);
        for (int i = 0; i <= N; ++i)
        {
            float t = (float)i / (float)N;
            float ang = angStart + (angEnd - angStart) * t;
            float cx = std::cos(ang), sz = std::sin(ang);
            outerTop[i] = glm::vec3(arcCentre.x + rOuter * cx, topY,
                                    arcCentre.z + rOuter * sz);
            innerTop[i] = glm::vec3(arcCentre.x + rInner * cx, topY,
                                    arcCentre.z + rInner * sz);
            outerBot[i] = glm::vec3(arcCentre.x + rOuter * cx, botY,
                                    arcCentre.z + rOuter * sz);
            innerBot[i] = glm::vec3(arcCentre.x + rInner * cx, botY,
                                    arcCentre.z + rInner * sz);
        }

        for (int i = 0; i < N; ++i)
        {
            float tMid = ((float)i + 0.5f) / (float)N;
            float angMid = angStart + (angEnd - angStart) * tMid;
            glm::vec3 radialOut(std::cos(angMid), 0.0f, std::sin(angMid));

            // Top of the kerb (always +Y normal)
            pushQuad(innerTop[i], outerTop[i], outerTop[i+1], innerTop[i+1],
                     glm::vec3(0, 1, 0), COLOR_KERB);
            // Bottom (resting on fairway top, normal -Y)
            pushQuad(innerBot[i], innerBot[i+1], outerBot[i+1], outerBot[i],
                     glm::vec3(0, -1, 0), COLOR_KERB);
            // Larger-radius face (outward-radial)
            pushQuad(outerBot[i], outerBot[i+1], outerTop[i+1], outerTop[i],
                     outwardIsLargerR ? radialOut : -radialOut, COLOR_KERB);
            // Smaller-radius face (inward-radial)
            pushQuad(innerBot[i+1], innerBot[i], innerTop[i], innerTop[i+1],
                     outwardIsLargerR ? -radialOut : radialOut, COLOR_KERB);
        }

        // End caps so the ring doesn't look hollow where it meets the straights.
        auto endCap = [&](int i, bool atStart)
        {
            // Quad at angle index i, joining inner & outer at that ring step.
            glm::vec3 a = innerBot[i];
            glm::vec3 b = outerBot[i];
            glm::vec3 c = outerTop[i];
            glm::vec3 d = innerTop[i];
            // Tangent at this angle (perpendicular to the radial direction).
            float ang = angStart + (angEnd - angStart) * ((float)i / (float)N);
            glm::vec3 tangent(-std::sin(ang), 0.0f, std::cos(ang));
            glm::vec3 nrm = atStart ? -tangent : tangent;
            // If sweep is from larger angle to smaller (clockwise), the
            // "start" end actually faces +tangent, and we'd need to flip.
            // Detect direction:
            if (angEnd < angStart) nrm = -nrm;

            if (atStart)
                pushQuad(a, b, c, d, nrm, COLOR_KERB);
            else
                pushQuad(b, a, d, c, nrm, COLOR_KERB);
        };
        endCap(0, true);
        endCap(N, false);
    };

    // Outer kerb (larger-radius arc — convex side of the bend)
    buildRing(arcR + halfW, arcR + halfW + kerbT, /*outwardIsLargerR=*/true);
    // Inner kerb (smaller-radius arc — concave side of the bend)
    // Note: rInner here is the larger of the two radii because the kerb sits
    // INSIDE the fairway-inner edge, i.e. closer to the arc centre.
    buildRing(arcR - halfW - kerbT, arcR - halfW, /*outwardIsLargerR=*/false);
}

// =============================================================================
// End wall — closing boundary at the cup end of the fairway
// =============================================================================
//
// A flat slab standing upright across the full width of the hole (fairway +
// both kerbs). It sits flush against the cup-end edge of the fairway slab,
// is as tall as the kerbs, and thick enough to be clearly visible from above.

void Hole13::buildEndWall(const glm::vec3& centre,
                          const glm::vec3& segDir,
                          float halfW,
                          float kerbT,
                          float wallT,
                          float wallH)
{
    glm::vec3 perp = perpXZ(segDir);   // perpendicular to segDir in XZ

    float span = halfW + kerbT;   // half-span including kerbs

    // Near edge (flush with the end of the slab) and far edge (wallT behind it)
    glm::vec3 botL  = centre + span * perp;
    glm::vec3 botR  = centre - span * perp;
    glm::vec3 botLf = botL + wallT * segDir;
    glm::vec3 botRf = botR + wallT * segDir;

    float baseY = FAIRWAY_TOP_Y;
    float topY  = FAIRWAY_TOP_Y + wallH;

    glm::vec3 TL  (botL.x,  topY,  botL.z);
    glm::vec3 TR  (botR.x,  topY,  botR.z);
    glm::vec3 TLf (botLf.x, topY,  botLf.z);
    glm::vec3 TRf (botRf.x, topY,  botRf.z);
    glm::vec3 BL  (botL.x,  baseY, botL.z);
    glm::vec3 BR  (botR.x,  baseY, botR.z);
    glm::vec3 BLf (botLf.x, baseY, botLf.z);
    glm::vec3 BRf (botRf.x, baseY, botRf.z);

    // Inner face (facing the tee, normal = -segDir)
    pushQuad(BR, BL, TL, TR, -segDir, COLOR_KERB);
    // Outer face (back of wall, normal = +segDir)
    pushQuad(BLf, BRf, TRf, TLf, segDir, COLOR_KERB);
    // Top face
    pushQuad(TL, TR, TRf, TLf, glm::vec3(0, 1, 0), COLOR_KERB);
    // Left side (+perp)
    pushQuad(BLf, BL, TL, TLf, perp, COLOR_KERB);
    // Right side (-perp)
    pushQuad(BR, BRf, TRf, TR, -perp, COLOR_KERB);
    // Bottom (hidden, faces down)
    pushQuad(BL, BLf, BRf, BR, glm::vec3(0, -1, 0), COLOR_KERB);
}

// =============================================================================
// Flag (pole + triangular pennant)
// =============================================================================

void Hole13::buildFlag(const glm::vec3& base, float poleH, float poleR,
                       const glm::vec3& poleColour,
                       const glm::vec3& flagColour)
{
    // Pole: square-prism approximation of a cylinder.
    glm::vec3 mn(base.x - poleR, base.y,         base.z - poleR);
    glm::vec3 mx(base.x + poleR, base.y + poleH, base.z + poleR);
    buildBox(mn, mx, poleColour);

    // Flag: triangular pennant in the +X direction, double-sided.
    float flagW = 0.9f;
    float flagH = 0.55f;
    glm::vec3 fA(base.x,          base.y + poleH - flagH,        base.z);
    glm::vec3 fB(base.x + flagW,  base.y + poleH - flagH * 0.5f, base.z);
    glm::vec3 fC(base.x,          base.y + poleH,                base.z);

    // Front (normal +Z)
    {
        unsigned int idx = (unsigned int)mVertices.size();
        mVertices.push_back({fA, { 0, 0,  1}, flagColour});
        mVertices.push_back({fB, { 0, 0,  1}, flagColour});
        mVertices.push_back({fC, { 0, 0,  1}, flagColour});
        mIndices.push_back(idx + 0);
        mIndices.push_back(idx + 1);
        mIndices.push_back(idx + 2);
    }
    // Back (normal -Z, reversed winding)
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

void Hole13::init(GLuint shaderProgram)
{
    if (mInitialised) return;

    mProgram = shaderProgram;

    // Cache uniform locations (names from src/shaders/vertex.glsl +
    // fragment.glsl).
    // NOTE: uMVP was removed when we moved view + projection to be
    // per-frame uniforms set by dev.cpp. uLightColor / uAmbient are also
    // set by dev.cpp now (from the Skybox), so this hole no longer caches
    // them either — the only per-frame thing left here is sunDir + camPos +
    // the per-hole model + normal matrix.
    mLocModel      = glGetUniformLocation(mProgram, "uModel");
    mLocNormalMat  = glGetUniformLocation(mProgram, "uNormalMat");
    mLocLightDir   = glGetUniformLocation(mProgram, "uLightDir");
    mLocViewPos    = glGetUniformLocation(mProgram, "uViewPos");

    mVertices.clear();
    mIndices.clear();

    // ── 1. Fairway slab (top + bottom + sides, with filleted bend) ──────────
    buildFairwaySlab();

    // ── 2. Kerbs ────────────────────────────────────────────────────────────
    // Recompute the same geometry parameters used inside buildFairwaySlab so
    // the kerbs line up with the slab edges (tangent points, arc centre, etc).
    // Keeping the parameters mirrored here means the slab and its kerbs are
    // built from the *same* underlying numbers — change one, both update.
    float halfInterior = glm::radians(BEND_INTERIOR_DEG * 0.5f);
    float R = BEND_FILLET_RADIUS;
    float T = R / std::tan(halfInterior);
    float C = R / std::sin(halfInterior);

    glm::vec3 bend(0.0f, FAIRWAY_TOP_Y, 0.0f);
    glm::vec3 arcCentre(0.0f, FAIRWAY_TOP_Y, C);
    glm::vec3 dirA( std::sin(halfInterior), 0.0f, std::cos(halfInterior));
    glm::vec3 dirB(-std::sin(halfInterior), 0.0f, std::cos(halfInterior));
    glm::vec3 perpA = perpXZ(dirA);
    glm::vec3 perpB = -perpXZ(dirB);

    glm::vec3 tanA = bend + T * dirA;
    glm::vec3 tanB = bend + T * dirB;

    // Each straight kerb runs from the tangent point outward to the segment
    // end. Length is the original SEGMENT_LEN minus the tangent distance T.
    // The kerbs are built FROM the tangent point IN THE DIRECTION segDir
    // (which already points from bend outward — i.e. away from the arc).
    float straightKerbLen = SEGMENT_LEN - T;

    buildKerbsForSegment(tanA, dirA, perpA, straightKerbLen,
                         FAIRWAY_HALF, KERB_HEIGHT, KERB_THICKNESS);
    buildKerbsForSegment(tanB, dirB, perpB, straightKerbLen,
                         FAIRWAY_HALF, KERB_HEIGHT, KERB_THICKNESS);

    // Arc kerbs along the fillet, using the same angle convention as
    // buildFairwaySlab (angTanA → angTanB, sweeping clockwise through 60°).
    float angTanA = std::atan2(tanA.z - arcCentre.z, tanA.x - arcCentre.x);
    float angTanB = std::atan2(tanB.z - arcCentre.z, tanB.x - arcCentre.x);
    buildArcKerbs(arcCentre, R, FAIRWAY_HALF,
                  angTanA, angTanB, ARC_SEGMENTS_FILLET,
                  KERB_HEIGHT, KERB_THICKNESS);

    // ── 3. Tee patch at the tee end (darker square on top of fairway) ───────
    {
        const float eps = 0.001f;
        glm::vec3 c = mTeeEndCentre;
        glm::vec3 mn(c.x - TEE_HALF_SIZE,
                     c.y + eps,
                     c.z - TEE_HALF_SIZE);
        glm::vec3 mx(c.x + TEE_HALF_SIZE,
                     c.y + 0.02f + eps,
                     c.z + TEE_HALF_SIZE);
        buildBox(mn, mx, COLOR_TEE);
    }

    // ── 4. Cup + flag at the cup end ────────────────────────────────────────
    // Pull the cup inward (away from the end wall) so it doesn't clip through.
    // The wall's inner face sits at mCupEndCentre; offset by CUP_RADIUS + a
    // small clearance gap back along -dirB (toward the bend).
    {
        float halfInt = glm::radians(BEND_INTERIOR_DEG * 0.5f);
        glm::vec3 dB(-std::sin(halfInt), 0.0f, std::cos(halfInt));
        const float CUP_WALL_CLEARANCE = 0.3f;   // gap between cup edge and wall
        glm::vec3 c = mCupEndCentre - (CUP_RADIUS + CUP_WALL_CLEARANCE) * dB;

        glm::vec3 cmn(c.x - CUP_RADIUS,
                      c.y - CUP_DEPTH,
                      c.z - CUP_RADIUS);
        glm::vec3 cmx(c.x + CUP_RADIUS,
                      c.y + 0.002f,
                      c.z + CUP_RADIUS);
        buildBox(cmn, cmx, COLOR_CUP);

        glm::vec3 poleBase(c.x, FAIRWAY_TOP_Y, c.z);
        buildFlag(poleBase, POLE_HEIGHT, POLE_RADIUS,
                  COLOR_POLE, COLOR_FLAG);
    }

    // ── 5. Closing boundary wall at the cup end ──────────────────────────────
    // dirB points from the bend toward the cup end — use the same value as
    // computed above (segment B's direction, already in scope).
    {
        // Recompute dirB (same formula as above, no new state needed).
        float halfInt = glm::radians(BEND_INTERIOR_DEG * 0.5f);
        glm::vec3 dB(-std::sin(halfInt), 0.0f, std::cos(halfInt));
        buildEndWall(mCupEndCentre,
                     dB,
                     FAIRWAY_HALF,
                     KERB_THICKNESS,
                     /*wallT=*/KERB_THICKNESS,
                     /*wallH=*/KERB_HEIGHT);
    }

    // ── 6. Upload to GPU ────────────────────────────────────────────────────
    if (mVertices.empty() || mIndices.empty())
    {
        std::cerr << "Hole13::init — no geometry was generated!" << std::endl;
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

    // Hardcoded camera framing (no AABB tracking).
    mCenter = SCENE_CENTER;
    mRadius = SCENE_RADIUS;

    // Free CPU-side copy now that it's on the GPU.
    mVertices.clear(); mVertices.shrink_to_fit();
    mIndices.clear();  mIndices.shrink_to_fit();

    mInitialised = true;
}

// =============================================================================
// draw — push uniforms and issue the draw call
// =============================================================================

void Hole13::draw(const glm::mat4& view,
                  const glm::mat4& proj,
                  const glm::vec3& sunDir,
                  const glm::vec3& camPos)
{
    if (!mInitialised) return;

    glUseProgram(mProgram);

    // Per-hole transform — model is identity for Hole13 (geometry is already
    // in world space). uModel + uNormalMat are the only matrices THIS hole
    // owns; view and projection are program-wide uniforms set by dev.cpp
    // once per frame, before any hole's draw() is called.
glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(80.0f, 0.35f, 15.0f));
    glm::mat3 nrmM  = glm::mat3(glm::transpose(glm::inverse(model)));

    if (mLocModel      != -1) glUniformMatrix4fv(mLocModel,     1, GL_FALSE, glm::value_ptr(model));
    if (mLocNormalMat  != -1) glUniformMatrix3fv(mLocNormalMat, 1, GL_FALSE, glm::value_ptr(nrmM));

    // Sun direction + camera position still come from the caller. uLightColor
    // and uAmbient are NOT set here anymore — dev.cpp computes them from the
    // Skybox each frame and sets them on the program for all holes to share.
    if (mLocLightDir   != -1) glUniform3fv(mLocLightDir,        1, glm::value_ptr(sunDir));
    if (mLocViewPos    != -1) glUniform3fv(mLocViewPos,         1, glm::value_ptr(camPos));

    // view + proj are program-wide (set by dev.cpp); suppress unused-param warnings.
    (void)view; (void)proj;

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// =============================================================================
// cleanup — release GL resources
// =============================================================================

void Hole13::cleanup()
{
    if (mEBO) { glDeleteBuffers(1, &mEBO); mEBO = 0; }
    if (mVBO) { glDeleteBuffers(1, &mVBO); mVBO = 0; }
    if (mVAO) { glDeleteVertexArrays(1, &mVAO); mVAO = 0; }
    mIndexCount = 0;
    mInitialised = false;
}