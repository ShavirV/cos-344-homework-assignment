#pragma once
#include "../core/Mesh.h"

// =============================================================================
// Hole 16 — "The Island"
// =============================================================================
//
// Layout (top-down, all units relative to local origin before world placement):
//
//                         ┌──────────────────────────────────┐
//                         │                                  │
//    STREAM (curved) ─────┤   C-SHAPED FAIRWAY (hilly)      │
//    separates tee from   │                                  │
//    green approach        │     [ WATER MOAT ]              │
//                         │   [ TREE ISLAND centre ]        │
//                         │     [ WATER MOAT ]              │
//                         │                                  │
//                         │   raised green at east tip       │
//                         └──────────────────────────────────┘
//
// The fairway is a C-arc running from the south-west tee, curving north,
// then east to the raised green at the end of the arc. The inner void of
// the C contains the tree island (a raised disc) surrounded by a water
// moat. A curved stream crosses the gap between the tee and the green,
// marking the boundary of the hole. Natural terrain undulation is baked
// into the arc surface via a sinusoidal height function so the fairway
// rolls and crests rather than lying flat.
//
// Geometry breakdown
// ------------------
//   [0]  Arc fairway          makeArcFairway()    — hilly C-shape
//   [1]  Arc border outer     makeArcBorder()     — outer kerb
//   [2]  Arc border inner     makeArcBorder()     — inner kerb
//   [3]  Raised green island  makeCylinder()      — frustum-like raised end
//   [4]  Green surface        makeCylinder()      — flat top of green
//   [5]  Green border ring    makeCylinder()      — kerb around green
//   [6]  Tree island base     makeCylinder()      — raised disc for tree
//   [7]  Stream segment 0     makeStreamSegment() — curved stream, seg 0
//   [8]  Stream segment 1     makeStreamSegment() — curved stream, seg 1
//   [9]  Stream segment 2     makeStreamSegment() — curved stream, seg 2
//   [10] Stream segment 3     makeStreamSegment() — curved stream, seg 3
//   [11] Stream segment 4     makeStreamSegment() — curved stream, seg 4
//   [12] Tee marker           makeCylinder()      — small flat disc at start
//   [13] Rock obstacle A      makeRock()
//   [14] Rock obstacle B      makeRock()
//   [15] Rock obstacle C      makeRock()
//   [16] Hill bump A          makeHill()
//   [17] Hill bump B          makeHill()
//   [18] Hill bump C          makeHill()
//   [19] Bridge deck          makeCuboid()        — over stream gap
//   [20] Bridge rail L        makeCuboid()
//   [21] Bridge rail R        makeCuboid()
//   -- water moat placeholder --   (see .cpp comments)
//
// MAX_MESHES is generous; adjust down once layout is finalised.
//
class Hole16
{
public:
    static const int MAX_MESHES = 32;

    Hole16();
    ~Hole16();

    // Non-copyable — meshes own GPU buffers.
    Hole16(const Hole16&)            = delete;
    Hole16& operator=(const Hole16&) = delete;

    // Draw every mesh belonging to this hole.
    void draw(GLuint shaderProgram, bool wireframe = false);

    // Apply a world-space offset to every mesh.
    // Call once after construction to place the hole in the global scene.
    void translate(float x, float y, float z);

    // Rotate every mesh about the world Y axis by `angle` degrees.
    // Call before translate() to spin the hole in place about its local origin.
    void rotate(float angle, float ax, float ay, float az);

    // Uniform scale — useful for fitting into the global course grid.
    void scale(float sx, float sy, float sz);

private:
    Mesh*  meshes[MAX_MESHES];
    int    meshCount;

    void addMesh(Mesh* m);

    // ── Internal builders ────────────────────────────────────────────────────
    // All coordinates are in local hole space (origin = centre of the C arc).
    // The arc sweeps from startAngleDeg to endAngleDeg in the XZ plane.

    // C-shaped fairway surface with per-vertex height variation.
    // innerR / outerR : inner and outer radii of the arc band.
    // startAngle / endAngle : arc sweep in radians (CCW from +X).
    // ringSegs : subdivisions around the arc.
    // radialSegs : subdivisions across the band width.
    static Mesh makeArcFairway(float innerR, float outerR,
                               float startAngle, float endAngle,
                               int ringSegs, int radialSegs);

    // Solid kerb ring matching the arc — a thin extruded strip.
    // side = +1 for outer edge, -1 for inner edge.
    static Mesh makeArcBorder(float radius, float borderThickness,
                              float borderHeight,
                              float startAngle, float endAngle,
                              int segs, int side);

    // One quad-strip segment of a curved stream.
    // The stream is approximated as a sequence of short cuboid slabs whose
    // centres follow a cubic Bezier; each slab is rotated to face the curve
    // tangent direction.
    // p0..p3 : Bezier control points in XZ (Y is fixed = streamY).
    // t0, t1 : parameter range [0,1] for this segment's portion of the curve.
    // width   : stream width (perpendicular to travel).
    static Mesh makeStreamSegment(
        glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3,
        float t0, float t1,
        float streamY, float width);
};