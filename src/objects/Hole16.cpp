#include "Hole16.h"
#include <cmath>
#include <functional>

// =============================================================================
// Constants — all in local hole space
// =============================================================================

// ── Arc geometry ─────────────────────────────────────────────────────────────
// Arc sweeps CCW from 0° (east, +X) to 330° (south-east).
// This leaves a 30° gap between 330° and 360°/0° which is where the stream
// and bridge cross — the "mouth" of the C.
//
//         330°  (green end)
//          ╲
//   (C arc) ╲___________) 0° (tee end, east)
//
//            ISLAND inside
//
static const float ARC_INNER_R   = 10.0f;
static const float ARC_OUTER_R   = 19.0f;
static const float ARC_START_DEG =  0.0f;   // tee end
static const float ARC_END_DEG   = 330.0f;  // green approach end
static const int   ARC_RING_SEGS = 80;
static const int   ARC_RAD_SEGS  = 10;

// ── Fairway height parameters ─────────────────────────────────────────────────
static const float HILL_FREQ_A  = 5.5f;
static const float HILL_AMP_A   = 1.0f;
static const float HILL_FREQ_B  = 9.8f;
static const float HILL_AMP_B   = 0.4f;
static const float CROWN_AMP    = 0.25f;

// ── Border paver geometry ─────────────────────────────────────────────────────
// BORDER_H: height the paver rises above the fairway surface.
// PAVER_W:  depth of the paver slab (radial thickness).
// The paver extends downward from the fairway surface all the way to Y=0
// so it is never floating or partially underground.
static const float BORDER_H     = 0.35f;
static const float PAVER_W      = 0.45f;

// ── Green (raised, at arc end tip ~330°) ─────────────────────────────────────
// Arc mid-radius at 330°: (10+19)/2 = 14.5  cos(330°)=0.866  sin(330°)=-0.5
static const float GREEN_R      = 5.5f;
static const float GREEN_Y      = 2.1f;
static const float GREEN_WALL_H = 2.2f;   // tall enough to reach ground from GREEN_Y
static const float GREEN_X      = 12.56f; // (ARC_INNER+ARC_OUTER)/2 * cos(330°)
static const float GREEN_Z      = -7.25f; // (ARC_INNER+ARC_OUTER)/2 * sin(330°)

// ── Tree island (inside the C, near origin) ───────────────────────────────────
static const float ISLAND_R      = 6.0f;
static const float ISLAND_Y      = 0.8f;
static const float ISLAND_WALL_H = 1.0f;
static const float ISLAND_X      = 2.0f;
static const float ISLAND_Z      = 0.0f;

// ── Water placeholder Y ───────────────────────────────────────────────────────
// TODO: Replace with animated Water mesh between ISLAND_R and (ARC_INNER_R-0.5)
// centred at (ISLAND_X, WATER_Y, ISLAND_Z) when water system is implemented.
static const float WATER_Y       = -0.15f;

// ── Stream ────────────────────────────────────────────────────────────────────
// Crosses the 30° gap between the tee (0°) and green (330°) arc ends.
// Bezier bows outward (+X direction, away from the island) for a natural curve.
//
// P0: just past the tee-side arc end  (outward, slightly +Z)
// P1: handle pulling the curve east   (+X)
// P2: handle on the green side        (+X, pulled toward green tip)
// P3: just past the green-side end    (green approach)
//
static const float    STREAM_Y  = -0.10f;
static const float    STREAM_W  =  3.5f;
static const int      STREAM_SEGS = 6;
static const glm::vec2 STREAM_P0 = { 14.5f,  2.0f  };
static const glm::vec2 STREAM_P1 = { 21.0f, -1.5f  };
static const glm::vec2 STREAM_P2 = { 20.0f, -6.5f  };
static const glm::vec2 STREAM_P3 = { 12.6f, -9.5f  };

// ── Bridge ────────────────────────────────────────────────────────────────────
// Sits at the midpoint of the stream, spanning it.
// Centre ≈ midpoint between tee-side and green-side arc ends.
static const float BRIDGE_X     = 17.73f;  // (19.0 + 16.45) / 2
static const float BRIDGE_Z     = -4.75f;  // (0.0  + -9.5 ) / 2
static const float BRIDGE_ROT   = -105.0f; // aligned to stream direction
static const float BRIDGE_L     =  8.0f;   // longer than stream width
static const float BRIDGE_W     =  4.5f;
static const float BRIDGE_Y     =  0.08f;

// ── Tee disc ──────────────────────────────────────────────────────────────────
// Arc start = 0°, mid-radius = 14.5, so tee is at (14.5, 0).
static const float TEE_R        =  2.2f;
static const float TEE_Y        =  0.05f;
static const float TEE_X        = 14.5f;   // (ARC_INNER+ARC_OUTER)/2 * cos(0°)
static const float TEE_Z        =  0.0f;

// ── Rock obstacles (arc-angle, radial-fraction, scale) ────────────────────────
struct RockSpec { float angle; float radialFrac; float scale; };
static const RockSpec ROCKS[] = {
    {  60.0f, 0.55f, 1.1f  },
    { 150.0f, 0.35f, 0.85f },
    { 240.0f, 0.70f, 1.3f  },
};

// ── Hill bumps (arc-angle, radial-fraction, height) ───────────────────────────
struct HillSpec { float angle; float radialFrac; float h; };
static const HillSpec HILLS[] = {
    { 100.0f, 0.50f, 1.0f },
    { 200.0f, 0.60f, 0.8f },
    { 280.0f, 0.40f, 1.2f },
};

// =============================================================================
// Local helpers
// =============================================================================

static glm::vec2 bezier(glm::vec2 p0, glm::vec2 p1,
                         glm::vec2 p2, glm::vec2 p3, float t)
{
    float u = 1.0f - t;
    return u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
}

static glm::vec2 bezierTangent(glm::vec2 p0, glm::vec2 p1,
                                glm::vec2 p2, glm::vec2 p3, float t)
{
    float u = 1.0f - t;
    return 3.0f*(u*u*(p1-p0) + 2.0f*u*t*(p2-p1) + t*t*(p3-p2));
}

// Height function — encodes this hole's unique terrain character.
// arcT    [0,1]: position along the arc sweep
// radialT [0,1]: position across the band (0=inner edge, 1=outer edge)
static float fairwayHeight(float arcT, float radialT)
{
    float h = HILL_AMP_A * sinf(arcT * HILL_FREQ_A * 2.0f * (float)M_PI)
            + HILL_AMP_B * sinf(arcT * HILL_FREQ_B * 2.0f * (float)M_PI + 1.1f);
    float crown = CROWN_AMP * (1.0f - fabsf(radialT * 2.0f - 1.0f));
    return h + crown;
}

// =============================================================================
// Hole16::makeArcFairway
// =============================================================================
// Builds the hilly C-shaped ribbon surface.
//
// heightFn: called as heightFn(arcT, radialT) to get the Y at each vertex.
//   Pass fairwayHeight for the hilly arc, or a lambda returning 0.0f for a
//   flat ground-level shadow copy (used for the base infill plane).
//
Mesh Hole16::makeArcFairway(float innerR, float outerR,
                            float startAngle, float endAngle,
                            int ringSegs, int radialSegs,
                            const std::function<float(float,float)>& heightFn)
{
    Mesh mesh(0.15f, 0.55f, 0.12f, 1.0f);

    float startRad = startAngle * (float)M_PI / 180.0f;
    float endRad   = endAngle   * (float)M_PI / 180.0f;
    int cols = radialSegs + 1;
    int rows = ringSegs   + 1;

    for (int i = 0; i < rows; i++)
    {
        float arcT  = (float)i / ringSegs;
        float angle = startRad + (endRad - startRad) * arcT;
        float cosA  = cosf(angle);
        float sinA  = sinf(angle);

        for (int j = 0; j < cols; j++)
        {
            float radialT = (float)j / radialSegs;
            float radius  = innerR + (outerR - innerR) * radialT;
            float y       = heightFn(arcT, radialT);

            glm::vec3 pos    = { radius * cosA, y, radius * sinA };
            glm::vec3 dTheta = { -radius * sinA, 0.0f, radius * cosA };
            glm::vec3 dRad   = { cosA, 0.0f, sinA };
            glm::vec3 n      = glm::normalize(glm::cross(dRad, dTheta));
            if (n.y < 0.0f) n = -n;

            mesh.addVertex({ pos, n, { arcT * 4.0f, radialT * 2.0f } });
        }
    }

    for (int i = 0; i < ringSegs; i++)
        for (int j = 0; j < radialSegs; j++)
        {
            unsigned int tl = (unsigned int)( i      * cols + j     );
            unsigned int tr = (unsigned int)( i      * cols + j + 1 );
            unsigned int bl = (unsigned int)((i + 1) * cols + j     );
            unsigned int br = (unsigned int)((i + 1) * cols + j + 1 );
            mesh.addTriangle(tl, bl, tr);
            mesh.addTriangle(tr, bl, br);
        }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Hole16::makeArcBorder
// =============================================================================
// Builds a single merged mesh of all paver segments along the arc edge.
//
// Each paver is a full 3-D cuboid:
//   - Width  (arc direction) : arc-length of one segment
//   - Depth  (radial)        : PAVER_W
//   - Height                 : from Y=0 (ground) up to fairwayHeight + BORDER_H
//
// This gives the paver real depth (front/back/top/bottom faces) and ensures
// it never floats — it always reaches the ground regardless of terrain height.
//
// Because Mesh is non-copyable we merge all segment vertices into ONE Mesh
// rather than building separate meshes and combining. This is both more
// efficient (one VAO) and avoids the segments[0]-only bug.
//
Mesh Hole16::makeArcBorder(float radius, float borderWidth,
                           float borderRaise,
                           float startAngle, float endAngle,
                           int segs, int side,
                           const std::function<float(float,float)>& heightFn)
{
    Mesh mesh(0.76f, 0.74f, 0.70f, 1.0f);  // concrete / sandstone

    float startRad = startAngle * (float)M_PI / 180.0f;
    float endRad   = endAngle   * (float)M_PI / 180.0f;

    // The paver sits at radius + side*borderWidth/2 (centred on the edge line).
    float paverCentreR = radius + (float)side * borderWidth * 0.5f;

    for (int i = 0; i < segs; i++)
    {
        float t0 = (float)i       / segs;
        float t1 = (float)(i + 1) / segs;
        float a0   = startRad + (endRad - startRad) * t0;
        float a1   = startRad + (endRad - startRad) * t1;
        float aMid = (a0 + a1) * 0.5f;

        // Terrain height at the edge where this paver sits.
        float radialEdge = (side > 0) ? 1.0f : 0.0f;
        float surfY = heightFn((t0 + t1) * 0.5f, radialEdge);

        // Full paver height: from the ground (Y=0) up through the surface
        // and then borderRaise above it.  Clamp to at least borderRaise so
        // negative terrain heights don't produce zero-height pavers.
        float bottomY = 0.0f;
        float topY    = surfY + borderRaise;
        float paverH  = topY - bottomY;
        if (paverH < 0.05f) paverH = 0.05f;

        // Arc-chord length for this segment (approximation).
        float segArcLen = fabsf(a1 - a0) * paverCentreR * 1.02f; // 2% overlap

        // Centre of this paver in world space (before rotation).
        float cx = paverCentreR * cosf(aMid);
        float cz = paverCentreR * sinf(aMid);
        float cy = bottomY + paverH * 0.5f;   // vertically centred

        // Build the cuboid in local space: length along X, height along Y,
        // depth along Z (radial direction after rotation).
        // We need 8 corners, 6 faces, 12 triangles.
        float hw = segArcLen * 0.5f;   // half arc-length
        float hh = paverH    * 0.5f;   // half height
        float hd = borderWidth * 0.5f; // half radial depth

        // Rotated basis vectors: the paver's local X aligns with the arc tangent.
        float cosA = cosf(aMid), sinA = sinf(aMid);
        // tangent along arc (CCW): (-sinA, 0, cosA)
        // radial outward:          ( cosA, 0, sinA)
        glm::vec3 tang   = { -sinA, 0.0f,  cosA };  // arc direction
        glm::vec3 radial = {  cosA * (float)side, 0.0f, sinA * (float)side }; // paver depth dir
        glm::vec3 up     = {  0.0f, 1.0f,  0.0f };

        // 8 corners
        glm::vec3 centre = { cx, cy, cz };
        glm::vec3 c[8];
        for (int s = 0; s < 2; s++) {       // arc sign: -hw / +hw
            for (int v = 0; v < 2; v++) {   // vertical: -hh / +hh
                for (int d = 0; d < 2; d++) { // depth: -hd / +hd
                    float ts = (s == 0) ? -hw : hw;
                    float vs = (v == 0) ? -hh : hh;
                    float ds = (d == 0) ? -hd : hd;
                    c[s*4 + v*2 + d] = centre + tang*ts + up*vs + radial*ds;
                }
            }
        }
        // c layout:  [arc-][vert-][dep-] = 0
        //            [arc-][vert-][dep+] = 1
        //            [arc-][vert+][dep-] = 2
        //            [arc-][vert+][dep+] = 3
        //            [arc+][vert-][dep-] = 4
        //            [arc+][vert-][dep+] = 5
        //            [arc+][vert+][dep-] = 6
        //            [arc+][vert+][dep+] = 7

        // Helper: emit one quad face (CCW from outside).
        auto emitFace = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c2, glm::vec3 d,
                            glm::vec3 n)
        {
            unsigned int base = (unsigned int)mesh.vertexCount;
            mesh.addVertex({ a, n, {0.0f, 0.0f} });
            mesh.addVertex({ b, n, {1.0f, 0.0f} });
            mesh.addVertex({ c2,n, {1.0f, 1.0f} });
            mesh.addVertex({ d, n, {0.0f, 1.0f} });
            mesh.addTriangle(base+0, base+1, base+2);
            mesh.addTriangle(base+2, base+3, base+0);
        };

        glm::vec3 nTang   = tang;
        glm::vec3 nRadOut = radial;
        glm::vec3 nRadIn  = -radial;

        // Top face (+Y)
        emitFace(c[2], c[6], c[7], c[3],  up);
        // Bottom face (-Y)
        emitFace(c[1], c[5], c[4], c[0], -up);
        // Outer radial face (away from fairway)
        emitFace(c[1], c[3], c[7], c[5],  nRadOut);
        // Inner radial face (toward fairway)
        emitFace(c[4], c[6], c[2], c[0],  nRadIn);
        // Arc-start end cap
        emitFace(c[0], c[2], c[3], c[1], -nTang);
        // Arc-end end cap
        emitFace(c[5], c[7], c[6], c[4],  nTang);
    }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Hole16::makeStreamSegment
// =============================================================================
Mesh Hole16::makeStreamSegment(glm::vec2 p0, glm::vec2 p1,
                               glm::vec2 p2, glm::vec2 p3,
                               float t0, float t1,
                               float streamY, float width)
{
    float tMid = (t0 + t1) * 0.5f;

    glm::vec2 centre  = bezier(p0, p1, p2, p3, tMid);
    glm::vec2 tangent = bezierTangent(p0, p1, p2, p3, tMid);
    if (glm::length(tangent) < 1e-6f) tangent = glm::vec2(1.0f, 0.0f);
    tangent = glm::normalize(tangent);

    glm::vec2 pStart = bezier(p0, p1, p2, p3, t0);
    glm::vec2 pEnd   = bezier(p0, p1, p2, p3, t1);
    float segLen = glm::length(pEnd - pStart) * 1.12f;

    float headingDeg = glm::degrees(atan2f(tangent.y, tangent.x));

    // Placeholder water: flat blue slab at streamY.
    // TODO: replace with Water mesh when water system is implemented.
    Mesh slab = Mesh::makeCuboid(segLen, 0.12f, width,
                                 0.18f, 0.42f, 0.72f, 1.0f);
    slab.rotate(headingDeg, 0.0f, 1.0f, 0.0f);
    slab.translate(centre.x, streamY, centre.y);
    slab.setup();
    return slab;
}

// =============================================================================
// Constructor
// =============================================================================
Hole16::Hole16()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── [0] Hilly arc fairway ─────────────────────────────────────────────────
    {
        Mesh* fairway = new Mesh(makeArcFairway(
            ARC_INNER_R, ARC_OUTER_R,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, ARC_RAD_SEGS,
            fairwayHeight));
        addMesh(fairway);
    }

    // ── [1] Ground-level infill arc ───────────────────────────────────────────
    // A flat copy of the arc at Y=0 fills the visual gap below the hilly
    // fairway surface, giving the impression of solid terrain underneath.
    // Using the heightFn parameter eliminates the post-construction vertex
    // hack that was needed before.
    {
        Mesh* base = new Mesh(makeArcFairway(
            ARC_INNER_R, ARC_OUTER_R,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, ARC_RAD_SEGS,
            [](float, float){ return 0.05f; }));    // flat at Y=0, no hack needed
        base->setRGBA(0.04f, 0.79f, 0.16f, 1.0f); // geeen
        addMesh(base);
    }

    // ── [2] Outer kerb ────────────────────────────────────────────────────────
    // Full 3-D paver: reaches from Y=0 upward through the terrain surface.
    {
        Mesh* kerbOuter = new Mesh(makeArcBorder(
            ARC_OUTER_R, PAVER_W, BORDER_H,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, +1, fairwayHeight));
        addMesh(kerbOuter);
    }

    // ── [3] Outer kerb ground infill ──────────────────────────────────────────
    // Flat version of the outer border at Y=0, prevents gaps at ground level
    // where the terrain dips below zero.
    {
        Mesh* kerbOuterFlat = new Mesh(makeArcBorder(
            ARC_OUTER_R, PAVER_W, BORDER_H,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, +1,
            [](float, float){ return 0.0f; }));
        kerbOuterFlat->setRGBA(0.68f, 0.65f, 0.60f, 1.0f);
        addMesh(kerbOuterFlat);
    }

    // ── [4] Inner kerb ────────────────────────────────────────────────────────
    {
        Mesh* kerbInner = new Mesh(makeArcBorder(
            ARC_INNER_R, PAVER_W, BORDER_H,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, -1, fairwayHeight));
        addMesh(kerbInner);
    }

    // ── [5] Inner kerb ground infill ─────────────────────────────────────────
    {
        Mesh* kerbInnerFlat = new Mesh(makeArcBorder(
            ARC_INNER_R, PAVER_W, BORDER_H,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, -1,
            [](float, float){ return 0.0f; }));
        kerbInnerFlat->setRGBA(0.68f, 0.65f, 0.60f, 1.0f);
        addMesh(kerbInnerFlat);
    }

    // ── [6] Raised green — retaining wall ────────────────────────────────────
    // Cylinder wall reaching from Y=0 to GREEN_Y so the island never floats.
    {
        Mesh* wall = new Mesh(Mesh::makeCylinder(
            GREEN_R + 0.4f, GREEN_WALL_H, 32,
            0.68f, 0.62f, 0.52f, 1.0f));
        // Cylinder origin is at its centre, so offset down so base sits at Y=0
        wall->translate(GREEN_X, GREEN_WALL_H * 0.5f, GREEN_Z);
        wall->setup();
        addMesh(wall);
    }

    // ── [7] Raised green — playing surface ───────────────────────────────────
    {
        Mesh* green = new Mesh(Mesh::makeCylinder(
            GREEN_R, 0.18f, 32,
            0.08f, 0.62f, 0.10f, 1.0f));
        green->translate(GREEN_X, GREEN_Y, GREEN_Z);
        green->setup();
        addMesh(green);
    }

    // ── [8] Raised green — kerb ring ─────────────────────────────────────────
    {
        Mesh* gKerb = new Mesh(Mesh::makeCylinder(
            GREEN_R + 0.55f, 0.30f, 32,
            0.76f, 0.72f, 0.66f, 1.0f));
        gKerb->translate(GREEN_X, GREEN_Y + 0.06f, GREEN_Z);
        gKerb->setup();
        addMesh(gKerb);
    }

    // ── [9] Tree island — retaining wall ────────────────────────────────────
    {
        Mesh* iWall = new Mesh(Mesh::makeCylinder(
            ISLAND_R, ISLAND_WALL_H, 36,
            0.58f, 0.52f, 0.42f, 1.0f));
        iWall->translate(ISLAND_X, ISLAND_WALL_H * 0.5f, ISLAND_Z);
        iWall->setup();
        addMesh(iWall);
    }

    // ── [10] Tree island — top surface ───────────────────────────────────────
    // Blender tree asset to be placed at (ISLAND_X, ISLAND_Y + ISLAND_WALL_H, ISLAND_Z)
    // ── Water moat — animated water ───────────────────────────────────────────
    // Annular water between ISLAND_R and (ARC_INNER_R - 0.5), approximated
    // with a square plane sized to the outer diameter.
    {
        waterMoat = new WaterMesh();

        float moatDiam             = (ARC_INNER_R - 0.5f) * 3.0f;
        waterMoat->planeWidth      = moatDiam;
        waterMoat->planeLength     = moatDiam;
        waterMoat->gridResolution  = 64;

        // Calm standing water — very small waves, slow scroll.
        waterMoat->waves[0] = { glm::vec2( 1.0f,  0.0f), 0.05f, 5.0f };
        waterMoat->waves[1] = { glm::vec2( 0.0f,  1.0f), 0.04f, 4.0f };
        waterMoat->waves[2] = { glm::vec2( 0.7f,  0.7f), 0.03f, 3.0f };
        waterMoat->waves[3] = { glm::vec2(-0.5f,  0.5f), 0.02f, 2.0f };

        waterMoat->scrollSpeed0   = 0.04f;
        waterMoat->scrollSpeed1   = 0.03f;
        waterMoat->colorShallow   = glm::vec3(0.22f, 0.50f, 0.52f);
        waterMoat->colorDeep      = glm::vec3(0.03f, 0.14f, 0.24f);
        waterMoat->foamThreshold  = 0.15f;  // almost no foam — still water

        waterMoat->modelMatrix =
            glm::translate(glm::mat4(1.0f),
                        glm::vec3(ISLAND_X, WATER_Y + 0.01f, ISLAND_Z));

        waterMoat->init(
            "src/shaders/water.vert",
            "src/shaders/water.frag",
            "src/assets/textures/water_normal.bmp",
            "src/assets/textures/water_normal2.bmp",
            "src/assets/textures/water_foam.bmp"
        );
    }

    // ── [11] Large ground disc ────────────────────────────────────────────────
    // Fills under the entire hole so no gaps appear when viewed from any angle.
    // {
    //     Mesh* ground = new Mesh(Mesh::makeCylinder(
    //         ARC_OUTER_R + PAVER_W + 2.0f, 0.15f, 64,
    //         0.24f, 0.17f, 0.10f, 1.0f));
    //     ground->translate(0.0f, -0.08f, 0.0f);
    //     ground->setup();
    //     addMesh(ground);
    // }

    // ── [12] Tee marker ───────────────────────────────────────────────────────
    {
        Mesh* tee = new Mesh(Mesh::makeCylinder(
            TEE_R, 0.12f, 20,
            0.85f, 0.82f, 0.18f, 1.0f));   // yellow
        tee->translate(TEE_X, TEE_Y, TEE_Z);
        tee->setup();
        addMesh(tee);
    }

    // ── [13–15] Stream segments ───────────────────────────────────────────────
    // Cubic Bezier crossing the gap between tee (0°) and green (330°) arc ends.
    // Bows outward (+X) for a natural meander away from the island.
    {
        waterStream = new WaterMesh();

        // Stream runs along the Bezier, which bows in the +X direction.
        // Main flow is roughly south (−Z), biased slightly toward +X.
        waterStream->planeWidth     = STREAM_W + 0.5f;
        waterStream->planeLength    = 30.0f;
        waterStream->gridResolution = 12;

        waterStream->waves[0] = { glm::vec2( 0.1f, -1.0f), 0.12f, 4.0f };
        waterStream->waves[1] = { glm::vec2( 0.2f, -1.0f), 0.07f, 2.5f };
        waterStream->waves[2] = { glm::vec2(-0.1f, -1.0f), 0.05f, 1.8f };
        waterStream->waves[3] = { glm::vec2( 0.0f, -0.8f), 0.04f, 1.2f };

        waterStream->scrollSpeed0  = 0.22f;
        waterStream->scrollSpeed1  = 0.14f;
        waterStream->colorShallow  = glm::vec3(0.28f, 0.58f, 0.55f);
        waterStream->colorDeep     = glm::vec3(0.04f, 0.18f, 0.28f);
        waterStream->foamThreshold = 0.06f;

        // Position at the Bezier midpoint (t=0.5), rotated to align with tangent.
        glm::vec2 mid     = bezier(STREAM_P0, STREAM_P1, STREAM_P2, STREAM_P3, 0.5f);
        glm::vec2 tan     = glm::normalize(bezierTangent(
                                STREAM_P0, STREAM_P1, STREAM_P2, STREAM_P3, 0.5f));
        float headingDeg  = glm::degrees(atan2f(tan.y, tan.x));

        waterStream->modelMatrix =
            glm::rotate(
                glm::translate(glm::mat4(1.0f),
                            glm::vec3(mid.x, STREAM_Y + 0.02f, mid.y)),
                glm::radians(headingDeg),
                glm::vec3(0.0f, 1.0f, 0.0f));

        waterStream->init(
            "src/shaders/water.vert",
            "src/shaders/water.frag",
            "src/assets/textures/water_normal.bmp",
            "src/assets/textures/water_normal2.bmp",
            "src/assets/textures/water_foam.bmp"
        );
    }

    // ── [16] Bridge ───────────────────────────────────────────────────────────
    // Spans the stream at its midpoint, aligned to the stream direction.
    // BRIDGE_ROT aligns the deck with the tangent at t=0.5 of the Bezier.
    {
        Mesh* deck = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.20f, BRIDGE_W,
            0.52f, 0.33f, 0.14f, 1.0f));
        deck->rotate(BRIDGE_ROT, 0.0f, 1.0f, 0.0f);
        deck->translate(BRIDGE_X, BRIDGE_Y, BRIDGE_Z);
        deck->setup();
        addMesh(deck);

        // Left rail
        Mesh* railL = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.50f, 0.14f,
            0.42f, 0.26f, 0.10f, 1.0f));
        railL->rotate(BRIDGE_ROT, 0.0f, 1.0f, 0.0f);
        railL->translate(BRIDGE_X, BRIDGE_Y + 0.35f,
                         BRIDGE_Z - BRIDGE_W * 0.5f + 0.1f);
        railL->setup();
        addMesh(railL);

        // Right rail
        Mesh* railR = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.50f, 0.14f,
            0.42f, 0.26f, 0.10f, 1.0f));
        railR->rotate(BRIDGE_ROT, 0.0f, 1.0f, 0.0f);
        railR->translate(BRIDGE_X, BRIDGE_Y + 0.35f,
                         BRIDGE_Z + BRIDGE_W * 0.5f - 0.1f);
        railR->setup();
        addMesh(railR);
    }

    // ── [17–19] Rock obstacles ────────────────────────────────────────────────
    {
        for (int ri = 0; ri < 3; ri++)
        {
            const RockSpec& rs = ROCKS[ri];
            float a     = rs.angle * (float)M_PI / 180.0f;
            float r     = ARC_INNER_R + (ARC_OUTER_R - ARC_INNER_R) * rs.radialFrac;
            float arcT  = (rs.angle - ARC_START_DEG) / (ARC_END_DEG - ARC_START_DEG);
            float yOnArc = fairwayHeight(arcT, rs.radialFrac);

            Mesh* rock = new Mesh(Mesh::makeRock(rs.scale));
            rock->translate(r * cosf(a), yOnArc + rs.scale * 0.6f, r * sinf(a));
            rock->rotate(rs.angle * 0.37f, 0.0f, 1.0f, 0.0f);
            rock->setup();
            addMesh(rock);
        }
    }

    // ── [20–22] Hill bumps ────────────────────────────────────────────────────
    {
        for (int hi = 0; hi < 3; hi++)
        {
            const HillSpec& hs = HILLS[hi];
            float a     = hs.angle * (float)M_PI / 180.0f;
            float r     = ARC_INNER_R + (ARC_OUTER_R - ARC_INNER_R) * hs.radialFrac;
            float arcT  = (hs.angle - ARC_START_DEG) / (ARC_END_DEG - ARC_START_DEG);
            float yOnArc = fairwayHeight(arcT, hs.radialFrac);

            Mesh* bump = new Mesh(Mesh::makeHill(
                2.5f, 2.0f, 5.5f, 4.5f, hs.h,
                0.10f, 0.45f, 0.08f, 1.0f));
            bump->translate(r * cosf(a), yOnArc, r * sinf(a));
            bump->rotate(hs.angle * 0.6f, 0.0f, 1.0f, 0.0f);
            bump->setup();
            addMesh(bump);
        }
    }
}

// =============================================================================
// Destructor
// =============================================================================
Hole16::~Hole16()
{
    for (int i = 0; i < MAX_MESHES; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
    delete waterStream;
    delete waterMoat;
}

void Hole16::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole16::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);

    // Water drawn last — alpha blended, must come after all opaque geometry.
    if (waterStream)
        waterStream->draw(camView, camProj, camPos);
    if (waterMoat)
        waterMoat->draw(camView, camProj, camPos);
}

void Hole16::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole16::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}

void Hole16::scale(float sx, float sy, float sz)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->scale(sx, sy, sz);
}

void Hole16::update(float deltaTime)
{
    if (waterStream) waterStream->update(deltaTime);
    if (waterMoat)   waterMoat->update(deltaTime);
}

void Hole16::setCameraData(const glm::mat4& view,
                           const glm::mat4& proj,
                           const glm::vec3& pos)
{
    camView = view;
    camProj = proj;
    camPos  = pos;
}