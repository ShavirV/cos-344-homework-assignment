#include "Hole17.h"
#include <cmath>
#include <functional>

// =============================================================================
// Layout constants — all in local hole space (origin = tee centre)
// =============================================================================

// ── Fairway Bezier centreline (XZ) ───────────────────────────────────────────
// Runs left-to-right (+X). Minor bends keep the Z displacement well within the
// 15-unit budget. Total hole length ≈ 28 units.
static const glm::vec2 BEZ_P0 = {  0.0f,  0.0f };
static const glm::vec2 BEZ_P1 = {  8.0f, -2.2f };  // drifts left approaching cave
static const glm::vec2 BEZ_P2 = { 20.0f,  2.2f };  // drifts right exiting cave
static const glm::vec2 BEZ_P3 = { 28.0f,  0.0f };

static const float FAIRWAY_W     = 6.0f;   // total width (±3 from centreline)
static const float FAIRWAY_HALF  = FAIRWAY_W * 0.5f;
static const int   FAIRWAY_SEGS  = 30;     // longitudinal subdivisions

// ── Fairway bump height function parameters ───────────────────────────────────
// Light undulation — amplitude is intentionally low so the cave remains
// the visual centrepiece, not topographic drama.
static const float BUMP_AMP_A   = 0.38f;  // primary wave
static const float BUMP_FREQ_A  = 3.0f;   // cycles over full length
static const float BUMP_AMP_B   = 0.12f;  // secondary ripple
static const float BUMP_FREQ_B  = 7.0f;
static const float BUMP_CROWN   = 0.10f;  // gentle crown across width

// ── Border kerb ───────────────────────────────────────────────────────────────
static const float PAVER_W      = 0.45f;
static const float PAVER_H      = 0.35f;

// ── Cave dimensions ───────────────────────────────────────────────────────────
static const float CAVE_ENTRY_X =  8.0f;
static const float CAVE_EXIT_X  = 22.0f;
static const float CAVE_LEN     = CAVE_EXIT_X - CAVE_ENTRY_X;   // 14.0
static const float CAVE_HALF_W  =  3.0f;   // half the interior width
static const float CAVE_FULL_W  =  CAVE_HALF_W * 2.0f;          //  6.0
static const float CAVE_CEIL_Y  =  3.5f;   // interior ceiling height
static const float CAVE_WALL_T  =  1.5f;   // wall slab thickness
// The cave centreline follows the Bezier, so at x=15 (midpoint) the Z offset
// from the Bezier is ≈0. We place the cave symmetrically about the Bezier.
// The entry/exit arch face centreline Z is evaluated from the Bezier.
// For simplicity the INTERIOR walls are world-axis aligned — the small bend
// of the Bezier is absorbed by the fairway strip itself.
static const float CAVE_Z_CENTRE = 0.0f;  // interior centred on Z=0

// ── Arch face geometry ────────────────────────────────────────────────────────
static const float ARCH_R       = 4.0f;   // inner radius of semicircle opening
static const float ARCH_FRAME_W = 0.85f;  // stone frame ring thickness
static const float ARCH_WALL_H  = CAVE_CEIL_Y; // total wall height at arch face
static const int   ARCH_SEGS    = 16;     // semicircle subdivisions

// ── Rock dome (exterior hill over cave) ───────────────────────────────────────
static const float DOME_CX      = 15.0f;  // centre X (midpoint of cave)
static const float DOME_CZ      =  0.0f;
static const float DOME_W       = 16.0f;  // dome terrain width (X direction)
static const float DOME_D       = 6.80f;  // dome terrain depth (Z direction)
static const float DOME_PEAK    =  4.0f;  // peak height above ground
static const float DOME_OFFSET_X = DOME_CX - DOME_W * 0.5f;  // 3.0
static const float DOME_OFFSET_Z = DOME_CZ - DOME_D * 0.5f;  // -8.0
static const int   DOME_SEGS_X  = 28;
static const int   DOME_SEGS_Z  = 20;

// ── Stalactites ───────────────────────────────────────────────────────────────
struct StalSpec { float x; float z; float radius; float length; };
static const StalSpec STALACTITES[] = {
    { 10.5f, -1.2f, 0.30f, 1.8f },
    { 12.0f,  1.8f, 0.22f, 2.4f },
    { 14.5f, -0.5f, 0.35f, 1.4f },
    { 17.0f,  1.0f, 0.25f, 2.0f },
    { 19.5f, -1.5f, 0.28f, 1.6f },
};
static const int STAL_COUNT = 5;

// ── Fairway hill bumps (on the open sections only, not inside cave) ────────────
struct BumpSpec { float arcT; float radialT; float h; };
static const BumpSpec BUMPS[] = {
    { 0.12f, 0.5f, 0.55f },   // early approach
    { 0.82f, 0.4f, 0.50f },   // exit approach
    { 0.93f, 0.6f, 0.40f },   // near green
};
static const int BUMP_COUNT = 3;

// ── Boulders at cave mouth ────────────────────────────────────────────────────
// Three rocks framing the entry and one at exit.
struct BoulderSpec { float x; float z; float scale; float rotY; };
static const BoulderSpec BOULDERS[] = {
    { 6.8f, -3.8f, 1.0f,  20.0f },  // entry left
    { 6.8f,  3.8f, 0.85f, -15.0f }, // entry right
    { 23.5f, -3.5f, 0.90f, 40.0f }, // exit left
    { 23.5f,  3.5f, 0.75f, -30.0f },// exit right
};
static const int BOULDER_COUNT = 4;

// ── Tee / green / flag ────────────────────────────────────────────────────────
static const float TEE_X       =  0.0f;
static const float TEE_Z       =  0.0f;
static const float TEE_R       =  1.8f;
static const float GREEN_X     = 28.0f;
static const float GREEN_Z     =  0.0f;
static const float FLAG_POLE_H =  2.2f;
static const float HOLE_CUP_R  =  0.18f;

// =============================================================================
// Local helpers
// =============================================================================

static glm::vec2 evalBez(glm::vec2 p0, glm::vec2 p1,
                          glm::vec2 p2, glm::vec2 p3, float t)
{
    float u = 1.0f - t;
    return u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
}

static glm::vec2 evalBezTan(glm::vec2 p0, glm::vec2 p1,
                             glm::vec2 p2, glm::vec2 p3, float t)
{
    float u = 1.0f - t;
    return 3.0f*(u*u*(p1-p0) + 2.0f*u*t*(p2-p1) + t*t*(p3-p2));
}

// Height function for the fairway surface.
// arcT [0,1]: position along the Bezier (0=tee, 1=green).
// radialT [0,1]: position across the width (0=left edge, 1=right edge).
// Heights are suppressed inside the cave so the fairway floor is flat there.
static float fairwayHeight(float arcT, float radialT)
{
    // Suppress bumps inside cave: arcT≈0.286..0.786 maps to x=8..22 on a 28-unit hole
    float caveTStart = CAVE_ENTRY_X / 28.0f;   // 0.286
    float caveTEnd   = CAVE_EXIT_X  / 28.0f;   // 0.786
    float inCave = 0.0f;
    if (arcT > caveTStart && arcT < caveTEnd)
    {
        // Smoothly suppress bumps inside cave with a raised cosine window.
        float tNorm = (arcT - caveTStart) / (caveTEnd - caveTStart);
        inCave = 0.5f - 0.5f * cosf(tNorm * 2.0f * (float)M_PI);
        // inCave goes 0→1→0 over the cave span; we use it to lerp height to 0.
    }

    float h = BUMP_AMP_A * sinf(arcT * BUMP_FREQ_A * 2.0f * (float)M_PI)
            + BUMP_AMP_B * sinf(arcT * BUMP_FREQ_B * 2.0f * (float)M_PI + 0.8f);
    float crown = BUMP_CROWN * (1.0f - fabsf(radialT * 2.0f - 1.0f));
    float total = h + crown;

    // Blend toward 0 inside cave.
    return total * (1.0f - inCave);
}

// Rock dome height function (for makeHeightmapTerrain).
// nx,nz in [0,1] → Y height of the dome mesh vertex.
// Uses a smooth radial falloff to form a natural hill shape.
static float domeHeight(float nx, float nz)
{
    // Map to [-1,+1] centred on dome.
    float dx = nx * 2.0f - 1.0f;  // -1=left edge, +1=right edge
    float dz = nz * 2.0f - 1.0f;
    // Elliptical radial distance (X stretch slightly because cave is longer than wide)
    float r2 = (dx*dx * 0.6f) + (dz*dz);
    if (r2 >= 1.0f) return 0.0f;
    // Smooth raised cosine profile.
    float r  = sqrtf(r2);
    float h  = DOME_PEAK * (0.5f + 0.5f * cosf(r * (float)M_PI));
    // Add mild noise-like variation using harmonics.
    h += 0.6f * sinf(dx * 3.5f + 0.4f) * cosf(dz * 2.8f) * (1.0f - r);
    if (h < 0.0f) h = 0.0f;
    return h;
}

// =============================================================================
// Hole17::makePathFairway
// =============================================================================
// Builds a ribbon mesh extruded along the Bezier centreline.
// At each longitudinal step, we evaluate the centreline position and tangent,
// then compute the perpendicular (right-hand normal in XZ) and place two
// vertices at ±halfWidth from the centreline (+ any offsetZ bias).
// heightFn(arcT, radialT) sets Y at each vertex.
//
Mesh Hole17::makePathFairway(glm::vec2 p0, glm::vec2 p1,
                             glm::vec2 p2, glm::vec2 p3,
                             float width, int segs,
                             float offsetZ,
                             const std::function<float(float,float)>& heightFn,
                             float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);
    float halfW = width * 0.5f;

    int rows = segs + 1;
    for (int i = 0; i < rows; i++)
    {
        float arcT = (float)i / segs;
        glm::vec2 pos = evalBez(p0, p1, p2, p3, arcT);
        glm::vec2 tan = evalBezTan(p0, p1, p2, p3, arcT);
        if (glm::length(tan) < 1e-6f) tan = glm::vec2(1.0f, 0.0f);
        tan = glm::normalize(tan);

        // Right-hand perpendicular in XZ: rotate tan 90° CW → (tan.y, -tan.x)
        // but in our convention X=forward, Z=left/right, so perpendicular is:
        glm::vec2 perp = { tan.y, -tan.x };

        // Apply offset bias (for border strips placed beside the fairway).
        glm::vec2 centreBiased = pos + perp * offsetZ;

        // Left vertex (radialT=0), right vertex (radialT=1)
        glm::vec2 leftXZ  = centreBiased - perp * halfW;
        glm::vec2 rightXZ = centreBiased + perp * halfW;

        float yLeft  = heightFn(arcT, 0.0f);
        float yRight = heightFn(arcT, 1.0f);
        float yMid   = heightFn(arcT, 0.5f);

        // Normal: approximate from the cross-slope of this row and the travel dir.
        // For gentle bumps, (0,1,0) is close enough, but we compute the tilt
        // from the height difference across the strip.
        float dY    = yRight - yLeft;
        float dZw   = width;
        glm::vec3 n = glm::normalize(glm::vec3(0.0f, dZw, -dY));

        glm::vec3 lv = { leftXZ.x,  yLeft,  leftXZ.y  };
        glm::vec3 rv = { rightXZ.x, yRight, rightXZ.y };

        mesh.addVertex({ lv, n, { 0.0f, arcT } });
        mesh.addVertex({ rv, n, { 1.0f, arcT } });
    }

    for (int i = 0; i < segs; i++)
    {
        unsigned int tl = (unsigned int)(i * 2);
        unsigned int tr = (unsigned int)(i * 2 + 1);
        unsigned int bl = (unsigned int)((i + 1) * 2);
        unsigned int br = (unsigned int)((i + 1) * 2 + 1);
        mesh.addTriangle(tl, bl, tr);
        mesh.addTriangle(tr, bl, br);
    }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Hole17::makePathBorder
// =============================================================================
// Full 3-D paver kerb placed along one side of the Bezier fairway.
// All segments are merged into one Mesh — same approach as makeArcBorder.
//
Mesh Hole17::makePathBorder(glm::vec2 p0, glm::vec2 p1,
                            glm::vec2 p2, glm::vec2 p3,
                            float fairwayHalfWidth,
                            int segs, int side,
                            float paverW, float paverH,
                            const std::function<float(float,float)>& heightFn,
                            float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    // Emit one oriented quad face into the shared mesh.
    auto emitFace = [&](glm::vec3 a2, glm::vec3 b2,
                        glm::vec3 c2, glm::vec3 d2, glm::vec3 n)
    {
        unsigned int base = (unsigned int)mesh.vertexCount;
        mesh.addVertex({ a2, n, {0.0f, 0.0f} });
        mesh.addVertex({ b2, n, {1.0f, 0.0f} });
        mesh.addVertex({ c2, n, {1.0f, 1.0f} });
        mesh.addVertex({ d2, n, {0.0f, 1.0f} });
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    };

    for (int i = 0; i < segs; i++)
    {
        float t0   = (float)i       / segs;
        float t1   = (float)(i + 1) / segs;
        float tMid = (t0 + t1) * 0.5f;

        glm::vec2 pos = evalBez(p0, p1, p2, p3, tMid);
        glm::vec2 tan = evalBezTan(p0, p1, p2, p3, tMid);
        if (glm::length(tan) < 1e-6f) tan = glm::vec2(1.0f, 0.0f);
        tan = glm::normalize(tan);

        // Right-hand perp: (tan.y, -tan.x)
        glm::vec2 perp = { tan.y, -tan.x };

        // Paver is placed at fairwayHalfWidth + paverW/2 on the given side.
        float edgeDist = fairwayHalfWidth + paverW * 0.5f;
        glm::vec2 paverCentre2D = pos + perp * ((float)side * edgeDist);

        // Surface Y at the edge of the fairway.
        float radialEdge = (side > 0) ? 1.0f : 0.0f;
        float surfY = heightFn(tMid, radialEdge);

        float bottomY = 0.0f;
        float topY    = surfY + paverH;
        float height  = topY - bottomY;
        if (height < 0.05f) height = 0.05f;

        // Segment length approximation.
        glm::vec2 ps = evalBez(p0, p1, p2, p3, t0);
        glm::vec2 pe = evalBez(p0, p1, p2, p3, t1);
        float segLen = glm::length(pe - ps) * 1.02f;

        float cx = paverCentre2D.x;
        float cz = paverCentre2D.y;
        float cy = bottomY + height * 0.5f;

        // Oriented basis vectors.
        glm::vec3 tang3   = glm::normalize(glm::vec3(tan.x,  0.0f, tan.y));
        glm::vec3 perp3   = glm::normalize(glm::vec3(perp.x * (float)side,
                                                      0.0f,
                                                      perp.y * (float)side));
        glm::vec3 up      = { 0.0f, 1.0f, 0.0f };
        glm::vec3 centre  = { cx, cy, cz };

        float hw = segLen  * 0.5f;
        float hh = height  * 0.5f;
        float hd = paverW  * 0.5f;

        glm::vec3 c[2][2][2];
        for (int ai=0;ai<2;ai++)
        for (int vi=0;vi<2;vi++)
        for (int di=0;di<2;di++)
            c[ai][vi][di] = centre
                          + tang3 * ((ai==0)?-hw:hw)
                          + up    * ((vi==0)?-hh:hh)
                          + perp3 * ((di==0)?-hd:hd);

        emitFace(c[0][1][0],c[1][1][0],c[1][1][1],c[0][1][1],  up);      // top
        emitFace(c[0][0][1],c[1][0][1],c[1][0][0],c[0][0][0], -up);      // bottom
        emitFace(c[0][0][1],c[0][1][1],c[1][1][1],c[1][0][1],  perp3);   // outer
        emitFace(c[1][0][0],c[1][1][0],c[0][1][0],c[0][0][0], -perp3);   // inner
        emitFace(c[0][0][0],c[0][1][0],c[0][1][1],c[0][0][1], -tang3);   // cap start
        emitFace(c[1][0][1],c[1][1][1],c[1][1][0],c[1][0][0],  tang3);   // cap end
    }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Hole17::makeArchFace
// =============================================================================
// Builds the stone arch face at one end of the cave.
//
// The arch is a semicircle of inner radius archR. The face mesh consists of:
//   A) The arch ring: quads connecting inner semicircle to outer semicircle.
//      This is the decorative stone frame around the opening.
//   B) Side panels: vertical rectangular slabs from the arch springer (Y=0)
//      to the ceiling (Y=wallH), on either side of the semicircle.
//   C) Top panel:   horizontal slab from the arch crown to the ceiling,
//      spanning the full wall width.
//
// All quads are emitted into a single Mesh. The face normal points along ±X.
//
Mesh Hole17::makeArchFace(float faceX, float faceCentreZ,
                          float archR, float frameW,
                          float wallH, float wallW,
                          int segs, bool facingPositiveX,
                          float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float nx  = facingPositiveX ? 1.0f : -1.0f;
    glm::vec3 norm = { nx, 0.0f, 0.0f };

    float rOuter = archR + frameW;
    float halfW  = wallW * 0.5f;

    // Helper: emit a quad (two CCW triangles) into mesh.
    auto emitQuad = [&](glm::vec3 a2, glm::vec3 b2,
                        glm::vec3 c2, glm::vec3 d2,
                        glm::vec3 n)
    {
        unsigned int base = (unsigned int)mesh.vertexCount;
        mesh.addVertex({ a2, n, {0,0} });
        mesh.addVertex({ b2, n, {1,0} });
        mesh.addVertex({ c2, n, {1,1} });
        mesh.addVertex({ d2, n, {0,1} });
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    };

    // ── A) Arch ring (semicircle frame) ───────────────────────────────────────
    // Sweeps from angle=0 (rightmost springer, +Z) to angle=PI (leftmost springer, -Z)
    // via the crown at angle=PI/2 (top, +Y).
    //  z = archR * cos(angle)  [right (+Z) at 0, left (−Z) at PI]
    //  y = archR * sin(angle)  [0 at springer, archR at crown]
    for (int i = 0; i < segs; i++)
    {
        float a0 = (float)i       / segs * (float)M_PI;
        float a1 = (float)(i + 1) / segs * (float)M_PI;

        // Inner arch edge positions (face-plane only, constant X = faceX).
        glm::vec3 iA = { faceX, archR  * sinf(a0),  faceCentreZ + archR  * cosf(a0) };
        glm::vec3 iB = { faceX, archR  * sinf(a1),  faceCentreZ + archR  * cosf(a1) };
        // Outer arch edge positions.
        glm::vec3 oA = { faceX, rOuter * sinf(a0),  faceCentreZ + rOuter * cosf(a0) };
        glm::vec3 oB = { faceX, rOuter * sinf(a1),  faceCentreZ + rOuter * cosf(a1) };

        // Quad: inner-A, inner-B, outer-B, outer-A (CCW for given norm direction).
        if (facingPositiveX)
            emitQuad(iA, iB, oB, oA, norm);
        else
            emitQuad(oA, oB, iB, iA, norm);
    }

    // ── B) Left side panel (from left springer down to floor, up to ceiling) ──
    // Left springer = arch position at angle=PI → z = faceCentreZ − archR, y = 0
    // The side panel fills from z=−halfW to z=−archR (to the left of the arch).
    float springerZLeft  = faceCentreZ - archR;   // where arch meets wall (left)
    float springerZRight = faceCentreZ + archR;   // (right)

    // Left side panel: x=faceX, z from -halfW to springerZLeft, y from 0 to wallH
    {
        glm::vec3 bl = { faceX, 0.0f, faceCentreZ - halfW  };
        glm::vec3 br = { faceX, 0.0f, springerZLeft        };
        glm::vec3 tr = { faceX, wallH, springerZLeft        };
        glm::vec3 tl = { faceX, wallH, faceCentreZ - halfW  };
        if (facingPositiveX) emitQuad(bl, br, tr, tl, norm);
        else                 emitQuad(tl, tr, br, bl, norm);
    }

    // Right side panel: z from springerZRight to +halfW
    {
        glm::vec3 bl = { faceX, 0.0f, springerZRight       };
        glm::vec3 br = { faceX, 0.0f, faceCentreZ + halfW  };
        glm::vec3 tr = { faceX, wallH, faceCentreZ + halfW  };
        glm::vec3 tl = { faceX, wallH, springerZRight       };
        if (facingPositiveX) emitQuad(bl, br, tr, tl, norm);
        else                 emitQuad(tl, tr, br, bl, norm);
    }

    // ── C) Top panel (above arch crown, full width) ────────────────────────────
    // Arch crown is at y = archR (+ outer makes oR). Panel goes from y=rOuter to y=wallH
    // spanning the full wall width.
    if (rOuter < wallH)
    // {
    //     glm::vec3 bl = { faceX, rOuter, faceCentreZ - halfW };
    //     glm::vec3 br = { faceX, rOuter, faceCentreZ + halfW };
    //     glm::vec3 tr = { faceX, wallH,  faceCentreZ + halfW };
    //     glm::vec3 tl = { faceX, wallH,  faceCentreZ - halfW };
    //     if (facingPositiveX) emitQuad(bl, br, tr, tl, norm);
    //     else                 emitQuad(tl, tr, br, bl, norm);
    // }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Hole17::makeArchFillPanel
// =============================================================================
// Fills the rectangular gap between the arch's shoulder (at Z = ±archR) and
// the full wall edge (Z = ±wallHalfW) on a given side, between Y=archTopY
// and Y=ceilY. This is a vertical rectangular slab facing ±X.
// side=-1: left fill panel, side=+1: right fill panel.
//
Mesh Hole17::makeArchFillPanel(float faceX, float faceCentreZ,
                               float archR, float wallHalfW,
                               float archTopY, float ceilY,
                               int side,
                               float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float z0, z1;
    if (side < 0) {
        z0 = faceCentreZ - wallHalfW;
        z1 = faceCentreZ - archR;
    } else {
        z0 = faceCentreZ + archR;
        z1 = faceCentreZ + wallHalfW;
    }

    glm::vec3 n = { -1.0f, 0.0f, 0.0f };   // inward (toward cave interior)

    unsigned int base = (unsigned int)mesh.vertexCount;
    mesh.addVertex({ {faceX, archTopY, z0}, n, {0,0} });
    mesh.addVertex({ {faceX, archTopY, z1}, n, {1,0} });
    mesh.addVertex({ {faceX, ceilY,    z1}, n, {1,1} });
    mesh.addVertex({ {faceX, ceilY,    z0}, n, {0,1} });
    mesh.addTriangle(base+0, base+1, base+2);
    mesh.addTriangle(base+2, base+3, base+0);

    mesh.setup();
    return mesh;
}

// =============================================================================
// Constructor
// =============================================================================
Hole17::Hole17()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── [0] Hilly fairway surface ─────────────────────────────────────────────
    {
        Mesh* fairway = new Mesh(makePathFairway(
            BEZ_P0, BEZ_P1, BEZ_P2, BEZ_P3,
            FAIRWAY_W, FAIRWAY_SEGS,
            0.0f,           // no Z offset — this is the centreline ribbon
            fairwayHeight,
            0.15f, 0.55f, 0.12f, 1.0f));
        addMesh(fairway);
    }

    // ── [1] Flat fairway infill (Y=0) ─────────────────────────────────────────
    {
        Mesh* base = new Mesh(makePathFairway(
            BEZ_P0, BEZ_P1, BEZ_P2, BEZ_P3,
            FAIRWAY_W, FAIRWAY_SEGS,
            0.0f,
            [](float,float){ return 0.0f; },
            0.08f, 0.45f, 0.08f, 1.0f));
        addMesh(base);
    }

    // ── [2] Left border kerb (side = -1 → left / −Z side) ────────────────────
    {
        Mesh* kerbL = new Mesh(makePathBorder(
            BEZ_P0, BEZ_P1, BEZ_P2, BEZ_P3,
            FAIRWAY_HALF, FAIRWAY_SEGS, -1,
            PAVER_W, PAVER_H, fairwayHeight,
            0.76f, 0.74f, 0.70f, 1.0f));
        addMesh(kerbL);
    }

    // ── [3] Right border kerb (side = +1 → right / +Z side) ──────────────────
    {
        Mesh* kerbR = new Mesh(makePathBorder(
            BEZ_P0, BEZ_P1, BEZ_P2, BEZ_P3,
            FAIRWAY_HALF, FAIRWAY_SEGS, +1,
            PAVER_W, PAVER_H, fairwayHeight,
            0.76f, 0.74f, 0.70f, 1.0f));
        addMesh(kerbR);
    }

    // ── [4] Ground base plane ─────────────────────────────────────────────────
    {
        Mesh* ground = new Mesh(Mesh::makeCuboid(
            36.0f, 0.15f, 20.0f,
            0.22f, 0.16f, 0.10f, 1.0f));
        ground->translate(14.0f, -0.08f, 0.0f);
        ground->setup();
        addMesh(ground);
    }

    // =========================================================================
    // CAVE INTERIOR
    // =========================================================================
    // All cave interior parts are world-axis aligned (constant X or constant Z
    // faces), centred on CAVE_Z_CENTRE=0. The fairway Bezier's small Z drift
    // (max ±2.2) is handled by the fairway strip itself; the cave box is wide
    // enough (CAVE_FULL_W=6) to contain the fairway at any cross-section.

    // ── [5] Cave floor ────────────────────────────────────────────────────────
    // Continues the green turf colour inside the cave.
    {
        Mesh* caveFloor = new Mesh(Mesh::makeCuboid(
            CAVE_LEN, 0.15f, CAVE_FULL_W,
            0.10f, 0.40f, 0.09f, 1.0f));
        caveFloor->translate(
            CAVE_ENTRY_X + CAVE_LEN * 0.5f,
            0.0f,
            CAVE_Z_CENTRE);
        caveFloor->setup();
        addMesh(caveFloor);
    }

    // ── [6] Cave left wall ────────────────────────────────────────────────────
    // Vertical slab: x=ENTRY..EXIT, z=−CAVE_HALF_W−WALL_T..−CAVE_HALF_W,
    // y=0..CAVE_CEIL_Y. Inward face (normal +Z) is what the player sees.
    {
        Mesh* wallL = new Mesh(Mesh::makeCuboid(
            CAVE_LEN, CAVE_CEIL_Y, CAVE_WALL_T,
            0.30f, 0.28f, 0.26f, 1.0f));   // dark limestone
        wallL->translate(
            CAVE_ENTRY_X + CAVE_LEN * 0.5f,
            CAVE_CEIL_Y * 0.5f,
            CAVE_Z_CENTRE - CAVE_HALF_W - CAVE_WALL_T * 0.5f);
        wallL->setup();
        addMesh(wallL);
    }

    // ── [7] Cave right wall ───────────────────────────────────────────────────
    {
        Mesh* wallR = new Mesh(Mesh::makeCuboid(
            CAVE_LEN, CAVE_CEIL_Y, CAVE_WALL_T,
            0.30f, 0.28f, 0.26f, 1.0f));
        wallR->translate(
            CAVE_ENTRY_X + CAVE_LEN * 0.5f,
            CAVE_CEIL_Y * 0.5f,
            CAVE_Z_CENTRE + CAVE_HALF_W + CAVE_WALL_T * 0.5f);
        wallR->setup();
        addMesh(wallR);
    }

    // ── [8] Cave ceiling ──────────────────────────────────────────────────────
    // A flat slab: slightly wider than the interior to cover wall tops.
    // {
    //     Mesh* ceil = new Mesh(Mesh::makeCuboid(
    //         CAVE_LEN, 0.4f, CAVE_FULL_W + CAVE_WALL_T * 2.0f + 0.2f,
    //         0.25f, 0.23f, 0.22f, 1.0f));
    //     ceil->translate(
    //         CAVE_ENTRY_X + CAVE_LEN * 0.5f,
    //         CAVE_CEIL_Y + 0.2f,
    //         CAVE_Z_CENTRE);
    //     ceil->setup();
    //     addMesh(ceil);
    // }

    // ── [9] Cave entry arch face ──────────────────────────────────────────────
    // Stone frame ring at the entry mouth. The arch opening is a semicircle
    // of radius ARCH_R centred at z=CAVE_Z_CENTRE. facingPositiveX=true means
    // the normals point toward the outside (−X direction from inside).
    {
        Mesh* archEntry = new Mesh(makeArchFace(
            CAVE_ENTRY_X,
            CAVE_Z_CENTRE,
            ARCH_R, ARCH_FRAME_W,
            ARCH_WALL_H, CAVE_FULL_W,
            ARCH_SEGS, false,    // normals face −X (outward at entry)
            0.42f, 0.40f, 0.38f, 1.0f));
        addMesh(archEntry);
    }

    // ── [10] Cave entry fill wall left ────────────────────────────────────────
    // Fills the rectangular shoulder gap left of the arch on the entry face.
    {
        Mesh* fillEntryL = new Mesh(makeArchFillPanel(
            CAVE_ENTRY_X, CAVE_Z_CENTRE,
            ARCH_R, CAVE_HALF_W,
            ARCH_R + ARCH_FRAME_W,   // archTopY = outer arch crown
            CAVE_CEIL_Y,
            -1,                      // left side
            0.38f, 0.36f, 0.34f, 1.0f));
        addMesh(fillEntryL);
    }

    // ── [11] Cave entry fill wall right ──────────────────────────────────────
    {
        Mesh* fillEntryR = new Mesh(makeArchFillPanel(
            CAVE_ENTRY_X, CAVE_Z_CENTRE,
            ARCH_R, CAVE_HALF_W,
            ARCH_R + ARCH_FRAME_W,
            CAVE_CEIL_Y,
            +1,
            0.38f, 0.36f, 0.34f, 1.0f));
        addMesh(fillEntryR);
    }

    // ── [12] Cave exit arch face ──────────────────────────────────────────────
    {
        Mesh* archExit = new Mesh(makeArchFace(
            CAVE_EXIT_X,
            CAVE_Z_CENTRE,
            ARCH_R, ARCH_FRAME_W,
            ARCH_WALL_H, CAVE_FULL_W,
            ARCH_SEGS, true,     // normals face +X (outward at exit)
            0.42f, 0.40f, 0.38f, 1.0f));
        addMesh(archExit);
    }

    // ── [13] Cave exit fill wall left ────────────────────────────────────────
    {
        Mesh* fillExitL = new Mesh(makeArchFillPanel(
            CAVE_EXIT_X, CAVE_Z_CENTRE,
            ARCH_R, CAVE_HALF_W,
            ARCH_R + ARCH_FRAME_W,
            CAVE_CEIL_Y,
            -1,
            0.38f, 0.36f, 0.34f, 1.0f));
        addMesh(fillExitL);
    }

    // ── [14] Cave exit fill wall right ───────────────────────────────────────
    {
        Mesh* fillExitR = new Mesh(makeArchFillPanel(
            CAVE_EXIT_X, CAVE_Z_CENTRE,
            ARCH_R, CAVE_HALF_W,
            ARCH_R + ARCH_FRAME_W,
            CAVE_CEIL_Y,
            +1,
            0.38f, 0.36f, 0.34f, 1.0f));
        addMesh(fillExitR);
    }

    // =========================================================================
    // ROCK DOME (cave exterior hill)
    // =========================================================================

    // ── [15] Rock dome heightmap terrain ─────────────────────────────────────
    // makeHeightmapTerrain is centred at the origin of the grid; we translate
    // to position it over the cave. The lambda captures dome parameters.
    {
        Mesh* dome = new Mesh(Mesh::makeHeightmapTerrain(
            DOME_W, DOME_D,
            DOME_SEGS_X, DOME_SEGS_Z,
            domeHeight,
            0.32f, 0.30f, 0.28f, 1.0f));   // rocky grey-brown
        dome->translate(DOME_CX, 3.5f, DOME_CZ);
        dome->setup();
        addMesh(dome);
    }

    // // ── [16] Rock dome left skirt ─────────────────────────────────────────────
    // Thin vertical slab to close the gap between the dome edge and the ground
    // on the left side, preventing a visible seam at the dome perimeter.
    {
        Mesh* skirtL = new Mesh(Mesh::makeCuboid(
            DOME_W, 0.6f, 0.4f,
            0.28f, 0.26f, 0.24f, 1.0f));
        skirtL->translate(DOME_CX, 0.3f, DOME_CZ - DOME_D * 0.5f);
        skirtL->setup();
        addMesh(skirtL);
    }

    // ── [17] Rock dome right skirt ────────────────────────────────────────────
    {
        Mesh* skirtR = new Mesh(Mesh::makeCuboid(
            DOME_W, 0.6f, 0.4f,
            0.28f, 0.26f, 0.24f, 1.0f));
        skirtR->translate(DOME_CX, 0.3f, DOME_CZ + DOME_D * 0.5f);
        skirtR->setup();
        addMesh(skirtR);
    }

    // ── [18–22] Stalactites ───────────────────────────────────────────────────
    // Cones hanging from the ceiling. makeCone builds with apex at top and base
    // at bottom; we rotate 180° around X to flip them so they hang downward,
    // then translate so the base is at CAVE_CEIL_Y.
    {
        for (int s = 0; s < STAL_COUNT; s++)
        {
            const StalSpec& ss = STALACTITES[s];
            Mesh* stal = new Mesh(Mesh::makeCone(
                ss.radius, ss.length, 10,
                0.28f, 0.26f, 0.24f, 1.0f));
            // Flip downward: rotate 180° around Z axis so apex points −Y.
            stal->rotate(180.0f, 0.0f, 0.0f, 1.0f);
            // Translate: base at CAVE_CEIL_Y, apex hangs down by ss.length.
            stal->translate(ss.x, CAVE_CEIL_Y, ss.z);
            stal->setup();
            addMesh(stal);
        }
    }

    // ── [23] Tee marker ───────────────────────────────────────────────────────
    {
        Mesh* tee = new Mesh(Mesh::makeCylinder(
            TEE_R, 0.12f, 20,
            0.85f, 0.82f, 0.18f, 1.0f));
        tee->translate(TEE_X, 0.05f, TEE_Z);
        tee->setup();
        addMesh(tee);
    }

    // ── [24] Flag pole ────────────────────────────────────────────────────────
    {
        Mesh* pole = new Mesh(Mesh::makeCylinder(
            0.05f, FLAG_POLE_H, 8,
            0.85f, 0.85f, 0.85f, 1.0f));
        pole->translate(GREEN_X, 0.0f, GREEN_Z);
        pole->setup();
        addMesh(pole);
    }

    // ── [25] Flag mesh ────────────────────────────────────────────────────────
    {
        Mesh* flag = new Mesh(Mesh::makeCuboid(
            0.7f, 0.4f, 0.02f,
            0.85f, 0.10f, 0.08f, 1.0f));
        flag->translate(GREEN_X + 0.38f, FLAG_POLE_H * 0.85f, GREEN_Z);
        flag->setup();
        addMesh(flag);
    }

    // ── [26] Hole cup ─────────────────────────────────────────────────────────
    {
        Mesh* cup = new Mesh(Mesh::makeCylinder(
            HOLE_CUP_R, 0.08f, 16,
            0.05f, 0.05f, 0.05f, 1.0f));
        cup->translate(GREEN_X, 0.01f, GREEN_Z);
        cup->setup();
        addMesh(cup);
    }

    // ── [27–29] Fairway hill bumps (open sections only) ───────────────────────
    // Placed at arcT values that land outside the cave (caveTStart≈0.29..caveTEnd≈0.79).
    {
        for (int b = 0; b < BUMP_COUNT; b++)
        {
            const BumpSpec& bs = BUMPS[b];
            glm::vec2 pos = evalBez(BEZ_P0, BEZ_P1, BEZ_P2, BEZ_P3, bs.arcT);
            float yOnFairway = fairwayHeight(bs.arcT, bs.radialT);

            Mesh* bump = new Mesh(Mesh::makeHill(
                2.0f, 1.8f,     // top W, D
                4.5f, 4.0f,     // base W, D
                bs.h,
                0.10f, 0.44f, 0.08f, 1.0f));
            bump->translate(pos.x, yOnFairway, pos.y);
            bump->setup();
            addMesh(bump);
        }
    }

    // ── [30–33] Entry/exit boulders ───────────────────────────────────────────
    {
        for (int b = 0; b < BOULDER_COUNT; b++)
        {
            const BoulderSpec& bs = BOULDERS[b];
            Mesh* rock = new Mesh(Mesh::makeRock(bs.scale));
            rock->rotate(bs.rotY, 0.0f, 1.0f, 0.0f);
            rock->translate(bs.x, bs.scale * 0.55f, bs.z);
            rock->setup();
            addMesh(rock);
        }
    }
}

// =============================================================================
// Destructor
// =============================================================================
Hole17::~Hole17()
{
    for (int i = 0; i < MAX_MESHES; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole17::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole17::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole17::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole17::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}

void Hole17::scale(float sx, float sy, float sz)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->scale(sx, sy, sz);
}