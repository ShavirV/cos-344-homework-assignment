#pragma once
#include "../core/Mesh.h"
#include <functional>

// =============================================================================
// Hole 17 — "The Cave"
// =============================================================================
//
// Top-down layout (local space, X = primary travel direction):
//
//  Z
//  ^
//  |  ┌────────────────────────────────────────────────────────────┐
//  |  │  rock dome (visible outside, covers x≈4..26, z≈-8..+8)   │
//  |  │                                                            │
// +3──│── ╔══════════════════════════════════╗ ──────────────────── │
//  |  │   ║   CAVE INTERIOR (x=8..22)        ║                    │
// +0──│───║──────────────────────────────────║────────────────────│──→ X
//  |  │   ║  fairway continues inside        ║                    │
// -3──│── ╚══════════════════════════════════╝ ──────────────────── │
//  |  │                                                            │
//  |  └────────────────────────────────────────────────────────────┘
//     0        8                    22        28
//     tee   cave entry           cave exit   green
//
// The fairway follows a gentle cubic Bezier centreline:
//   Bends slightly LEFT (−Z) approaching the cave, slightly RIGHT (+Z) exiting.
//   Width: 6 world units total (±3 from centreline).
//
// Cave geometry (full 3-D):
//   Entry / exit arch faces  : semicircular arch ring with stone frame
//   Left wall                : vertical slab, floor→ceiling, full cave length
//   Right wall               : mirror of left wall
//   Ceiling                  : flat slab at CAVE_CEIL_Y, same width as cave
//   Fill walls (above arch)  : solid panels closing the gap between the round
//                              arch top and the rectangular ceiling level
//   Rock dome                : heightmap terrain mound covering the cave exterior
//   Stalactites              : cones hanging from the ceiling at irregular positions
//
// Mesh index reference
// ────────────────────
//  [ 0]  Fairway surface (Bezier path strip, light bumps)
//  [ 1]  Fairway flat infill (Y=0 base)
//  [ 2]  Left border kerb strip
//  [ 3]  Right border kerb strip
//  [ 4]  Ground base plane
//  [ 5]  Cave floor
//  [ 6]  Cave left wall
//  [ 7]  Cave right wall
//  [ 8]  Cave ceiling
//  [ 9]  Cave entry arch ring (stone frame face)
//  [10]  Cave entry fill wall L (above arch left side)
//  [11]  Cave entry fill wall R (above arch right side)
//  [12]  Cave exit arch ring
//  [13]  Cave exit fill wall L
//  [14]  Cave exit fill wall R
//  [15]  Rock dome (heightmap)
//  [16]  Rock dome left skirt
//  [17]  Rock dome right skirt
//  [18–22]  Stalactites 0–4
//  [23]  Tee marker disc
//  [24]  Flag pole
//  [25]  Flag mesh
//  [26]  Hole cup
//  [27–29]  Fairway hill bumps A–C
//  [30–32]  Entry/exit boulders
//
class Hole17
{
public:
    static const int MAX_MESHES = 48;   // generous — cave has many parts

    Hole17();
    ~Hole17();

    Hole17(const Hole17&)            = delete;
    Hole17& operator=(const Hole17&) = delete;

    void draw     (GLuint shaderProgram, bool wireframe = false);
    void translate(float x, float y, float z);
    void rotate   (float angle, float ax, float ay, float az);
    void scale    (float sx, float sy, float sz);

private:
    Mesh*  meshes[MAX_MESHES];
    int    meshCount;
    void   addMesh(Mesh* m);

    // ── Bezier path strip ─────────────────────────────────────────────────────
    // Builds the fairway surface by extruding a ribbon along a cubic Bezier
    // centreline, with per-vertex height via heightFn(arcT, radialT).
    // offsetZ: shifts the ribbon's centreline perpendicular to the curve by
    //          this amount — used to produce the left/right border kerbs.
    static Mesh makePathFairway(glm::vec2 p0, glm::vec2 p1,
                                glm::vec2 p2, glm::vec2 p3,
                                float width, int segs,
                                float offsetZ,
                                const std::function<float(float,float)>& heightFn,
                                float r, float g, float b, float a = 1.0f);

    // ── Path border kerb ──────────────────────────────────────────────────────
    // Full 3-D paver kerb following a Bezier centreline, offset to one side.
    // side = +1 right (+Z), -1 left (−Z), relative to the curve tangent.
    // paverW: radial depth of the paver.
    // paverH: height above the fairway surface (paver always extends to Y=0).
    static Mesh makePathBorder(glm::vec2 p0, glm::vec2 p1,
                               glm::vec2 p2, glm::vec2 p3,
                               float fairwayHalfWidth,
                               int segs, int side,
                               float paverW, float paverH,
                               const std::function<float(float,float)>& heightFn,
                               float r, float g, float b, float a = 1.0f);

    // ── Arch ring face ────────────────────────────────────────────────────────
    // Builds the decorative stone arch face at one end of the cave.
    // The arch is a semicircle of inner radius archR; outer ring is archR+frameW.
    // The face is a flat mesh (constant X = faceX) facing along ±X.
    // wallH: total wall height at this face (arch top to ceiling level).
    // wallW: total wall width (= cave interior width).
    // frameW: stone frame ring thickness.
    // segs: angular divisions for the semicircle.
    // facingPositiveX: true = entry face (normals toward −X = outside), else exit.
    static Mesh makeArchFace(float faceX, float faceCentreZ,
                             float archR, float frameW,
                             float wallH, float wallW,
                             int segs, bool facingPositiveX,
                             float r, float g, float b, float a = 1.0f);

    // ── Arch fill panel ───────────────────────────────────────────────────────
    // Flat rectangular panel filling the gap between the arch's rectangular
    // shoulder and the cave ceiling. One panel per side (left / right).
    // side = -1 left, +1 right.
    static Mesh makeArchFillPanel(float faceX, float faceCentreZ,
                                  float archR, float wallHalfW,
                                  float archTopY, float ceilY,
                                  int side,
                                  float r, float g, float b, float a = 1.0f);
};