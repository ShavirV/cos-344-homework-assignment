#pragma once
#include "../core/Mesh.h"
#include <functional>

// =============================================================================
// Hole 16 — "The Island"
// =============================================================================
//
// Top-down layout (local space, origin at arc centre):
//
//      0°  ← tee end (east, +X)
//       ╲
//        ╲  ←──── stream + bridge cross here ────→
//         )────────────────────────────────────)
//         (    hilly C-arc fairway              )
//         )────────────────────────────────────)
//            [ water moat — TODO ]
//         [ tree island disc — Blender tree here ]
//            [ water moat — TODO ]
//        ╱
//       ╱  330° ← green end (south-east)
//
// Mesh index reference
// ────────────────────
//   0   Hilly arc fairway (main playing surface)
//   1   Ground-level infill arc (Y=0, fills below hills)
//   2   Outer kerb — terrain-level, full 3-D paver
//   3   Outer kerb ground infill (Y=0)
//   4   Inner kerb — terrain-level, full 3-D paver
//   5   Inner kerb ground infill (Y=0)
//   6   Raised green retaining wall (cylinder, base at Y=0)
//   7   Raised green playing surface
//   8   Raised green kerb ring
//   9   Tree island retaining wall (base at Y=0)
//  10   Tree island top surface
//  11   Large ground disc (fills beneath entire hole)
//  12   Tee marker disc
//  13–18  Stream segments 0–5 (placeholder, replace with Water)
//  19   Bridge deck
//  20   Bridge rail L
//  21   Bridge rail R
//  22–24  Rock obstacles A, B, C
//  25–27  Hill bumps A, B, C
//
class Hole16
{
public:
    static const int MAX_MESHES = 32;

    Hole16();
    ~Hole16();

    Hole16(const Hole16&)            = delete;
    Hole16& operator=(const Hole16&) = delete;

    void draw     (GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate   (float angle, float ax, float ay, float az);
    void scale    (float sx, float sy, float sz);

private:
    Mesh*  meshes[MAX_MESHES];
    int    meshCount;
    void   addMesh(Mesh* m);

    // ── Arc fairway ───────────────────────────────────────────────────────────
    // heightFn(arcT, radialT) → Y value at each vertex.
    // Pass fairwayHeight for the hilly surface, or [](float,float){return 0.0f;}
    // for a flat ground-level infill copy — eliminates the vertex-hack pattern.
    static Mesh makeArcFairway(float innerR, float outerR,
                               float startAngle, float endAngle,
                               int ringSegs, int radialSegs,
                               const std::function<float(float,float)>& heightFn);

    // ── Arc border / kerb paver ───────────────────────────────────────────────
    // Builds ONE merged mesh containing all paver segments (fixes the
    // segments[0]-only bug). Each segment is a full 6-face cuboid:
    //   - Arc width   : chord length of that angular slice
    //   - Radial depth: borderWidth (PAVER_W)
    //   - Height      : from Y=0 to (heightFn(t,edge) + borderRaise)
    // side = +1 outer, -1 inner.
    // heightFn: same contract as makeArcFairway — use flat lambda for infill.
    static Mesh makeArcBorder(float radius, float borderWidth,
                              float borderRaise,
                              float startAngle, float endAngle,
                              int segs, int side,
                              const std::function<float(float,float)>& heightFn);

    // ── Stream segment ────────────────────────────────────────────────────────
    static Mesh makeStreamSegment(glm::vec2 p0, glm::vec2 p1,
                                  glm::vec2 p2, glm::vec2 p3,
                                  float t0, float t1,
                                  float streamY, float width);
};