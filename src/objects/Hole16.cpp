#include "Hole16.h"
#include <cmath>

// =============================================================================
// Constants — all in local hole space
// =============================================================================

// ── Arc geometry ──────────────────────────────────────────────────────────────
// The C-shape sweeps from 200° to 340° (CCW from +X in the XZ plane),
// opening toward the east (+X). That gives a natural C orientation where:
//   - Tee is at the south-west tip   (~200°)
//   - Green is at the north-west tip (~340°)
//   - The open mouth of the C faces east toward the tree island

static const float ARC_INNER_R   = 10.0f;   // inner edge of fairway band
static const float ARC_OUTER_R   = 19.0f;   // outer edge of fairway band
static const float ARC_START_DEG = 00.0f;  // tee end
static const float ARC_END_DEG   = 330.0f;  // green approach end
static const int   ARC_RING_SEGS = 80;      // angular subdivisions
static const int   ARC_RAD_SEGS  = 10;       // radial subdivisions (across width)

// ── Fairway height function parameters ───────────────────────────────────────
// Height is a sum of two sinusoids in the arc-length direction plus a gentle
// radial cross-slope, giving rolling hills along the course and a slight
// crown in the centre of the band. Peak-to-trough ≈ 1.8 world units.
static const float HILL_FREQ_A   = 5.5f;    // cycles over the full arc
static const float HILL_AMP_A    = 1.0f;    // metres
static const float HILL_FREQ_B   = 9.8f;    // higher-frequency ripple
static const float HILL_AMP_B    = 0.4f;
static const float CROWN_AMP     = 0.25f;   // cross-slope crown height

// ── Green (raised island at the north-west arc tip) ───────────────────────────
// The last ~20° of the arc rises onto a frustum-style raised disc.
static const float GREEN_R       = 5.5f;    // radius of the raised green disc
static const float GREEN_Y       = 2.0f;    // height above local Y=0
static const float GREEN_WALL_H  = 1.8f;    // retaining wall height
static const float GREEN_X       = 13.6f;   // local X position (north-west tip)
static const float GREEN_Z       = -5.0f;   // local Z position

// ── Tree island ───────────────────────────────────────────────────────────────
// Raised disc at the centre of the C, surrounded by a water moat.
// The actual tree is a Blender asset — only the island geometry is here.
static const float ISLAND_R      = 6.0f;
static const float ISLAND_Y      = 0.8f;    // slightly raised above fairway base
static const float ISLAND_WALL_H = 1.0f;
static const float ISLAND_X      = 2.0f;   // local centre (inside the C mouth)
static const float ISLAND_Z      = 0.0f;

// ── Water moat placeholder Y ─────────────────────────────────────────────────
// Water meshes are intentionally omitted here. When the water system is
// implemented, place an annular water surface at Y = WATER_Y between
// ISLAND_R and (ARC_INNER_R - 0.5) centred on (ISLAND_X, WATER_Y, ISLAND_Z).
static const float WATER_Y       = -0.15f;

// ── Stream ────────────────────────────────────────────────────────────────────
// The curved stream crosses the mouth of the C between the tee (south) and
// the green approach (north). It is approximated by 5 quad-slab segments
// following a cubic Bezier whose control points curve it slightly westward
// (into the page) creating a natural meander.
//
// Control points are in XZ, Y is fixed. The curve goes from south of the
// C mouth to north of it, bowing gently west.
static const float STREAM_Y      = -0.20f;
static const float STREAM_W      = 3.5f;

// Bezier control points (XZ):
//   P0 = south entry (near tee exit)
//   P1, P2 = interior handles pulling the curve westward
//   P3 = north exit (near green approach)
static const glm::vec2 STREAM_P0 = { 12.0f,  14.0f };
static const glm::vec2 STREAM_P1 = {  4.0f,   8.0f };
static const glm::vec2 STREAM_P2 = {  5.0f,  -8.0f };
static const glm::vec2 STREAM_P3 = { 12.0f, -14.0f };

static const int STREAM_SEGS     = 5;

// ── Bridge (over the stream, connecting path to green approach) ────────────────
static const float BRIDGE_Y      = 0.05f;
static const float BRIDGE_W      = 4.5f;
static const float BRIDGE_L      = 5.5f;
static const float BRIDGE_X      = 11.5f;
static const float BRIDGE_Z      = 0.0f;

// ── Rock obstacles ────────────────────────────────────────────────────────────
// Three boulders scattered around the outer arc at different angles.
struct RockSpec { float angle; float radialFrac; float scale; };
static const RockSpec ROCKS[] = {
    { 240.0f, 0.55f, 1.1f },   // south-west, mid-band
    { 275.0f, 0.35f, 0.85f },  // west, inner side
    { 315.0f, 0.7f,  1.3f },   // north-west, outer side
};

// ── Hill bumps ────────────────────────────────────────────────────────────────
// Additional makeHill bumps placed on the fairway surface for extra relief.
// (arc angle degrees, radial fraction 0=inner..1=outer, height)
struct HillSpec { float angle; float radialFrac; float h; };
static const HillSpec HILLS[] = {
    { 230.0f, 0.5f, 1.0f },
    { 265.0f, 0.6f, 0.8f },
    { 300.0f, 0.4f, 1.2f },
};

// ── Tee disc ─────────────────────────────────────────────────────────────────
static const float TEE_R         = 2.2f;
static const float TEE_Y         = 0.05f;
// Tee position = arc start tip
static const float TEE_ANGLE_RAD = ARC_START_DEG * (float)M_PI / 180.0f;
static const float TEE_X         = (ARC_INNER_R + ARC_OUTER_R) * 0.5f
                                    * cosf(TEE_ANGLE_RAD);
static const float TEE_Z         = (ARC_INNER_R + ARC_OUTER_R) * 0.5f
                                    * sinf(TEE_ANGLE_RAD);

// =============================================================================
// Helpers
// =============================================================================

// Evaluate a cubic Bezier at parameter t, returning an XZ position.
static glm::vec2 bezier(glm::vec2 p0, glm::vec2 p1,
                        glm::vec2 p2, glm::vec2 p3, float t)
{
    float u  = 1.0f - t;
    float u2 = u * u,  u3 = u2 * u;
    float t2 = t * t,  t3 = t2 * t;
    return u3*p0 + 3.0f*u2*t*p1 + 3.0f*u*t2*p2 + t3*p3;
}

// Evaluate the Bezier tangent (derivative) at t.
static glm::vec2 bezierTangent(glm::vec2 p0, glm::vec2 p1,
                               glm::vec2 p2, glm::vec2 p3, float t)
{
    float u = 1.0f - t;
    return 3.0f*(u*u*(p1-p0) + 2.0f*u*t*(p2-p1) + t*t*(p3-p2));
}

// Signed sinusoidal height for a fairway vertex.
// arcT   : [0,1] position along the arc (angular direction)
// radialT: [0,1] position across the band (0=inner, 1=outer)
static float fairwayHeight(float arcT, float radialT)
{
    float h  = HILL_AMP_A * sinf(arcT * HILL_FREQ_A * 2.0f * (float)M_PI)
             + HILL_AMP_B * sinf(arcT * HILL_FREQ_B * 2.0f * (float)M_PI + 1.1f);
    // Crown: highest in the middle of the band, slopes to edges.
    float crown = CROWN_AMP * (1.0f - fabsf(radialT * 2.0f - 1.0f));
    return h + crown;
}

// Compute an outward face normal for a quad given its four corners (CCW order).
static glm::vec3 quadNormal(glm::vec3 a, glm::vec3 b,
                            glm::vec3 c, glm::vec3 /*d*/)
{
    return glm::normalize(glm::cross(b - a, c - a));
}

// =============================================================================
// Mesh::makeArcFairway
// =============================================================================
// Builds a C-shaped ribbon mesh. For each ring step i and radial step j we
// compute:
//   angle  = lerp(startAngle, endAngle, i / ringSegs)
//   radius = lerp(innerR, outerR, j / radialSegs)
//   x      = radius * cos(angle)
//   z      = radius * sin(angle)
//   y      = fairwayHeight(i/ringSegs, j/radialSegs)
//
// Vertices are wound CCW when viewed from above (+Y), matching the cuboid
// convention used throughout this codebase.
//
Mesh Hole16::makeArcFairway(float innerR, float outerR,
                            float startAngle, float endAngle,
                            int ringSegs, int radialSegs)
{
    // Grass green — same visual language as makeHole's red but for fairway.
    Mesh mesh(0.15f, 0.55f, 0.12f, 1.0f);

    float startRad = startAngle * (float)M_PI / 180.0f;
    float endRad   = endAngle   * (float)M_PI / 180.0f;

    int cols = radialSegs + 1;   // vertices across the band
    int rows = ringSegs   + 1;   // vertices along the arc

    // ── Emit vertices ─────────────────────────────────────────────────────────
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
            float y       = fairwayHeight(arcT, radialT);

            glm::vec3 pos = { radius * cosA, y, radius * sinA };

            // Normal: approximate upward + outward component
            // (computed analytically from partial derivatives of the surface)
            // dPos/dTheta direction (tangent along arc):
            glm::vec3 dTheta = { -radius * sinA, 0.0f, radius * cosA };
            // dPos/dRadius direction (tangent across band):
            glm::vec3 dRad   = { cosA, 0.0f, sinA };
            // Normal = cross(dRad, dTheta), normalised
            glm::vec3 n = glm::normalize(glm::cross(dRad, dTheta));
            // Ensure normal points generally upward
            if (n.y < 0.0f) n = -n;

            glm::vec2 uv = { arcT * 4.0f, radialT * 2.0f };   // tiled UVs

            mesh.addVertex({ pos, n, uv });
        }
    }

    // ── Emit indices (CCW when viewed from above) ─────────────────────────────
    for (int i = 0; i < ringSegs; i++)
    {
        for (int j = 0; j < radialSegs; j++)
        {
            // Quad corners:
            //   tl — tr
            //   |       |
            //   bl — br
            unsigned int tl = (unsigned int)( i      * cols + j     );
            unsigned int tr = (unsigned int)( i      * cols + j + 1 );
            unsigned int bl = (unsigned int)((i + 1) * cols + j     );
            unsigned int br = (unsigned int)((i + 1) * cols + j + 1 );

            // Two CCW triangles (viewed from +Y):
            mesh.addTriangle(tl, bl, tr);
            mesh.addTriangle(tr, bl, br);
        }
    }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Mesh::makeArcBorder
// =============================================================================
// Thin vertical strip following the inner or outer edge of the arc.
// Each sector of the strip is a quad — two triangles — with a normal pointing
// radially outward (outer border) or inward (inner border).
//
Mesh Hole16::makeArcBorder(float radius, float borderThickness,
                           float borderHeight,
                           float startAngle, float endAngle,
                           int segs, int side)
{
    // Concrete / kerb colour
    Mesh mesh(0.78f, 0.76f, 0.72f, 1.0f);

    float startRad = startAngle * (float)M_PI / 180.0f;
    float endRad   = endAngle   * (float)M_PI / 180.0f;

    // The border is a thin band at `radius`, extruded outward by side*borderThickness
    // and raised by borderHeight. We emit one quad per arc segment.

    for (int i = 0; i < segs; i++)
    {
        float t0 = (float)i       / segs;
        float t1 = (float)(i + 1) / segs;
        float a0 = startRad + (endRad - startRad) * t0;
        float a1 = startRad + (endRad - startRad) * t1;

        // Height at the fairway surface for this arc position
        float yInner0 = fairwayHeight(t0, side > 0 ? 1.0f : 0.0f);
        float yInner1 = fairwayHeight(t1, side > 0 ? 1.0f : 0.0f);

        float rInner  = radius;
        float rOuter  = radius + side * borderThickness;

        // Four corners of the border quad (bottom of inner edge, top, etc.)
        // We build it as a vertical face aligned to the arc.
        glm::vec3 p0b = { rInner * cosf(a0), yInner0,              rInner * sinf(a0) };
        glm::vec3 p0t = { rInner * cosf(a0), yInner0 + borderHeight, rInner * sinf(a0) };
        glm::vec3 p1b = { rInner * cosf(a1), yInner1,              rInner * sinf(a1) };
        glm::vec3 p1t = { rInner * cosf(a1), yInner1 + borderHeight, rInner * sinf(a1) };

        // Outward-facing normal (averaged between the two angles)
        float aMid = (a0 + a1) * 0.5f;
        glm::vec3 n = glm::normalize(glm::vec3(
            (float)side * cosf(aMid), 0.0f, (float)side * sinf(aMid)));

        unsigned int base = (unsigned int)mesh.vertexCount;
        mesh.addVertex({ p0b, n, { t0, 0.0f } });
        mesh.addVertex({ p1b, n, { t1, 0.0f } });
        mesh.addVertex({ p1t, n, { t1, 1.0f } });
        mesh.addVertex({ p0t, n, { t0, 1.0f } });

        // CCW winding when viewed from the outside (normal direction)
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    }

    mesh.setup();
    return mesh;
}

// =============================================================================
// Mesh::makeStreamSegment
// =============================================================================
// One short rectangular slab representing a section of the curved stream.
// We evaluate the Bezier at the midpoint of [t0,t1], build a cuboid aligned
// to the tangent direction, then place it at that position.
//
// This gives a piecewise-linear approximation to the curve that looks smooth
// when STREAM_SEGS >= 5. The Y coordinate of all stream segments is fixed at
// streamY.
//
Mesh Hole16::makeStreamSegment(glm::vec2 p0, glm::vec2 p1,
                               glm::vec2 p2, glm::vec2 p3,
                               float t0, float t1,
                               float streamY, float width)
{
    float tMid = (t0 + t1) * 0.5f;

    // Centre of this segment
    glm::vec2 centre  = bezier(p0, p1, p2, p3, tMid);
    // Tangent direction (XZ), normalised
    glm::vec2 tangent = bezierTangent(p0, p1, p2, p3, tMid);
    if (glm::length(tangent) < 1e-6f) tangent = glm::vec2(0,1);
    tangent = glm::normalize(tangent);

    // Segment length = arc-length approximation between t0 and t1
    glm::vec2 pStart = bezier(p0, p1, p2, p3, t0);
    glm::vec2 pEnd   = bezier(p0, p1, p2, p3, t1);
    float segLen = glm::length(pEnd - pStart) * 1.15f;   // slight overlap to avoid gaps

    // Heading angle around Y: atan2(tangent.y, tangent.x)
    //   tangent.x corresponds to world X, tangent.y to world Z
    float headingDeg = glm::degrees(atan2f(tangent.y, tangent.x));

    // Water / stream colour — placeholder blue until water system is active
    Mesh slab = Mesh::makeCuboid(segLen, 0.15f, width,
                                 0.18f, 0.42f, 0.72f, 1.0f);

    // Rotate to align the slab's long axis (X) with the tangent direction
    slab.rotate(headingDeg, 0.0f, 1.0f, 0.0f);

    // Place at the curve midpoint
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

    // ── [0] Arc fairway ───────────────────────────────────────────────────────
    {
        Mesh* fairway = new Mesh(makeArcFairway(
            ARC_INNER_R, ARC_OUTER_R,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, ARC_RAD_SEGS));
        addMesh(fairway);

        {
        Mesh* flatFairway = new Mesh(makeArcFairway(
            ARC_INNER_R, ARC_OUTER_R,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS, ARC_RAD_SEGS));
        
        // Override the height function by directly modifying vertices to Y=0
        for (int i = 0; i < flatFairway->vertexCount; i++) {
            flatFairway->vertices[i].position.y = 0.0f;
            flatFairway->vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        flatFairway->setup();
        addMesh(flatFairway);
        }   
    }

    // ── [1] Arc border — outer kerb ───────────────────────────────────────────
    {
        Mesh* borderOuter = new Mesh(makeArcBorder(
            ARC_OUTER_R, 3.5f, 0.35f,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS,
            +1));   // side=+1: extrude radially outward
        addMesh(borderOuter);

        Mesh* flatBorderOuter = new Mesh(makeArcBorder(
            ARC_OUTER_R, 3.5f, 0.35f,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS,
            +1));
        
        // Override the height function by directly modifying vertices to Y=0
        for (int i = 0; i < flatBorderOuter->vertexCount; i++) {
            flatBorderOuter->vertices[i].position.y = 0.0f;
            flatBorderOuter->vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        flatBorderOuter->setup();
        addMesh(flatBorderOuter);
    }

    // ── [2] Arc border — inner kerb ───────────────────────────────────────────
    {
        Mesh* borderInner = new Mesh(makeArcBorder(
            ARC_INNER_R, 3.5f, 0.35f,
            ARC_START_DEG, ARC_END_DEG,
            ARC_RING_SEGS,
            -1));   // side=-1: extrude radially inward
        addMesh(borderInner);
    }

    // ── [3] Green retaining wall (frustum cylinder) ───────────────────────────
    // A short cylinder whose top face is at GREEN_Y forms the retaining wall.
    {
        Mesh* wall = new Mesh(Mesh::makeCylinder(
            GREEN_R + 0.4f,     // slightly wider than the green top
            GREEN_WALL_H, 28,
            0.68f, 0.62f, 0.52f, 1.0f));   // sandy stone colour
        wall->translate(GREEN_X, GREEN_Y - GREEN_WALL_H * 0.5f, GREEN_Z);
        wall->setup();
        addMesh(wall);
    }

    // ── [4] Green surface (raised disc) ──────────────────────────────────────
    // A very thin cylinder forms the top playing surface of the elevated green.
    {
        Mesh* green = new Mesh(Mesh::makeCylinder(
            GREEN_R, 0.18f, 28,
            0.08f, 0.60f, 0.10f, 1.0f));   // bright putting-green turf
        green->translate(GREEN_X, GREEN_Y, GREEN_Z);
        green->setup();
        addMesh(green);
    }

    // ── [5] Green border ring (kerb around the raised green) ─────────────────
    {
        Mesh* greenKerb = new Mesh(Mesh::makeCylinder(
            GREEN_R + 0.55f, 0.28f, 28,
            0.78f, 0.74f, 0.68f, 1.0f));   // concrete ring
        greenKerb->translate(GREEN_X, GREEN_Y + 0.05f, GREEN_Z);
        greenKerb->setup();
        // Hollow appearance: the inner green disc sits on top, hiding the centre.
        addMesh(greenKerb);
    }

    // ── [6] Tree island ───────────────────────────────────────────────────────
    // Raised disc. The Blender tree model will be placed at (ISLAND_X, ISLAND_Y
    // + ISLAND_WALL_H, ISLAND_Z) by the scene assembly code.
    // ── Water moat placeholder ────────────────────────────────────────────────
    // TODO (water system): Add an annular water mesh between ISLAND_R and
    // (ARC_INNER_R - 0.5f), centred at (ISLAND_X, WATER_Y, ISLAND_Z).
    // The moat also fills the open mouth of the C between the arc endpoints.
    {
        // Island retaining wall
        Mesh* islandWall = new Mesh(Mesh::makeCylinder(
            ISLAND_R, ISLAND_WALL_H, 32,
            0.58f, 0.52f, 0.42f, 1.0f));
        islandWall->translate(ISLAND_X,
                              ISLAND_Y - ISLAND_WALL_H * 0.5f,
                              ISLAND_Z);
        islandWall->setup();
        addMesh(islandWall);

        // Island top surface (earthy / soil colour — the tree sits here)
        Mesh* islandTop = new Mesh(Mesh::makeCylinder(
            ISLAND_R - 0.2f, 0.2f, 32,
            0.32f, 0.22f, 0.10f, 1.0f));
        islandTop->translate(ISLAND_X, ISLAND_Y + 0.1f, ISLAND_Z);
        islandTop->setup();
        addMesh(islandTop);
    }

    // ── [7–11] Curved stream ──────────────────────────────────────────────────
    // Five segments following the cubic Bezier defined by STREAM_P0..P3.
    // Each segment is a flat blue slab rotated to match the curve tangent.
    // TODO (water system): Replace these placeholder slabs with animated
    // water quads from the Water class when implemented.
    {
        for (int s = 0; s < STREAM_SEGS; s++)
        {
            float t0 = (float)s       / STREAM_SEGS;
            float t1 = (float)(s + 1) / STREAM_SEGS;
            Mesh* seg = new Mesh(makeStreamSegment(
                STREAM_P0, STREAM_P1, STREAM_P2, STREAM_P3,
                t0, t1,
                STREAM_Y, STREAM_W));
            addMesh(seg);
        }
    }

    // ── [12] Tee marker ───────────────────────────────────────────────────────
    {
        Mesh* tee = new Mesh(Mesh::makeCylinder(
            TEE_R, 0.12f, 20,
            0.85f, 0.85f, 0.20f, 1.0f));   // yellow tee marker
        tee->translate(TEE_X, TEE_Y, TEE_Z);
        tee->setup();
        addMesh(tee);
    }

    // ── [13–15] Rock obstacles ────────────────────────────────────────────────
    {
        for (int ri = 0; ri < 3; ri++)
        {
            const RockSpec& rs = ROCKS[ri];
            float a = rs.angle * (float)M_PI / 180.0f;
            float r = ARC_INNER_R + (ARC_OUTER_R - ARC_INNER_R) * rs.radialFrac;
            float arcT    = (rs.angle - ARC_START_DEG)
                            / (ARC_END_DEG - ARC_START_DEG);
            float yOnArc  = fairwayHeight(arcT, rs.radialFrac);

            Mesh* rock = new Mesh(Mesh::makeRock(rs.scale));
            rock->translate(r * cosf(a),
                            yOnArc + rs.scale * 0.6f,   // sit on surface
                            r * sinf(a));
            // Rotate each rock randomly so they don't all face the same way
            rock->rotate(rs.angle * 0.37f, 0.0f, 1.0f, 0.0f);
            rock->setup();
            addMesh(rock);
        }
    }

    // ── [16–18] Hill bumps ────────────────────────────────────────────────────
    // Additional makeHill shapes placed on the fairway for extra topographic
    // interest — rolling knolls the ball must navigate around.
    {
        for (int hi = 0; hi < 3; hi++)
        {
            const HillSpec& hs = HILLS[hi];
            float a = hs.angle * (float)M_PI / 180.0f;
            float r = ARC_INNER_R + (ARC_OUTER_R - ARC_INNER_R) * hs.radialFrac;
            float arcT   = (hs.angle - ARC_START_DEG)
                           / (ARC_END_DEG - ARC_START_DEG);
            float yOnArc = fairwayHeight(arcT, hs.radialFrac);

            // makeHill: top narrower than base for a natural-looking mound.
            Mesh* hill = new Mesh(Mesh::makeHill(
                2.5f, 2.0f,          // top width, depth
                5.5f, 4.5f,          // base width, depth
                hs.h,                // height
                0.12f, 0.48f, 0.09f, 1.0f));   // darker grass
            hill->translate(r * cosf(a), yOnArc, r * sinf(a));
            hill->rotate(hs.angle * 0.6f, 0.0f, 1.0f, 0.0f);
            hill->setup();
            addMesh(hill);
        }
    }

    // ── [19–21] Bridge ────────────────────────────────────────────────────────
    // A simple wooden bridge over the stream at the mouth of the C.
    // This is where the player crosses from the main course path to the
    // green approach side. The bridge aligns with the stream's midpoint.
    {
        // Deck
        Mesh* deck = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.18f, BRIDGE_W,
            0.52f, 0.33f, 0.14f, 1.0f));   // warm wood colour
        deck->translate(BRIDGE_X, BRIDGE_Y, BRIDGE_Z);
        deck->setup();
        addMesh(deck);

        // Left rail
        Mesh* railL = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.55f, 0.15f,
            0.42f, 0.26f, 0.10f, 1.0f));
        railL->translate(BRIDGE_X, BRIDGE_Y + 0.35f,
                         BRIDGE_Z - BRIDGE_W * 0.5f + 0.1f);
        railL->setup();
        addMesh(railL);

        // Right rail
        Mesh* railR = new Mesh(Mesh::makeCuboid(
            BRIDGE_L, 0.55f, 0.15f,
            0.42f, 0.26f, 0.10f, 1.0f));
        railR->translate(BRIDGE_X, BRIDGE_Y + 0.35f,
                         BRIDGE_Z + BRIDGE_W * 0.5f - 0.1f);
        railR->setup();
        addMesh(railR);
    }

    // ── Ground base (fills under the entire hole) ─────────────────────────────
    // A large flat disc at Y=0 so there are no gaps visible beneath the arc.
    // Coloured as rough ground / earth.
    {
        Mesh* base = new Mesh(Mesh::makeCylinder(
            ARC_OUTER_R + 2.0f, 0.15f, 48,
            0.28f, 0.20f, 0.12f, 1.0f));
        base->translate(0.0f, -0.12f, 0.0f);
        base->setup();
        addMesh(base);
    }
}

// =============================================================================
// Destructor
// =============================================================================

Hole16::~Hole16()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

// =============================================================================
// Private helpers
// =============================================================================

void Hole16::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

// =============================================================================
// Public interface
// =============================================================================

void Hole16::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
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