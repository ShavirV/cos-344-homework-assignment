#pragma once
// Mesh.h  —  full header (existing content preserved; new additions marked NEW)
//
// All object-generating helpers that were previously free functions in main.cpp
// are now static factory methods here so that every shape lives in Mesh.

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <functional>

// ── Vertex layout ─────────────────────────────────────────────────────────────
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

// ── Capacity constants ─────────────────────────────────────────────────────────
static const int MAX_VERTICES = 100000;
static const int MAX_INDICES  = 300000;
static const int MAX_WIRE     = MAX_INDICES * 6;   // 2 floats×3 per edge, 3 edges

class Mesh
{
public:
    // ── RGBA colour carried by this mesh ──────────────────────────────────────
    float r, g, b, a;

    // ── Model matrix (identity by default) ───────────────────────────────────
    glm::mat4 modelMatrix;

    // ── Selection state ───────────────────────────────────────────────────────
    bool select;

    // ── GPU handles ───────────────────────────────────────────────────────────
    GLuint VAO, VBO, EBO;
    GLuint wireVAO, wireVBO;
    bool   buffersInitialized;

    // ── CPU-side geometry ─────────────────────────────────────────────────────
    int          vertexCount;
    int          indexCount;
    int          wireVertexCount;
    Vertex*      vertices;
    unsigned int* indices;
    float*       wireVertices;

    // ── Life-cycle ────────────────────────────────────────────────────────────
    explicit Mesh(float r = 1, float g = 1, float b = 1, float a = 1);
    ~Mesh();
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Non-copyable
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    // ── Geometry helpers ──────────────────────────────────────────────────────
    void addVertex(const Vertex& v);
    void addIndex(unsigned int i);
    void addTriangle(unsigned int i0, unsigned int i1, unsigned int i2);

    // ── GPU operations ────────────────────────────────────────────────────────
    void setup();
    void cleanup();
    void buildWireVBO();
    void draw(GLuint shaderProgram, bool wireframe);

    // ── Transform helpers ─────────────────────────────────────────────────────
    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void scale(float sx, float sy, float sz);
    void resetTransform();
    void applyMatrix(const glm::mat4& m);

    // ── RGBA / selection ──────────────────────────────────────────────────────
    void setRGBA(float r, float g, float b, float a);
    void selected(bool s);
    bool isSelected();

    // =========================================================================
    // PRIMITIVE FACTORY METHODS  (existing)
    // =========================================================================
    static Mesh makeCuboid(float width, float height, float depth,
                           float r, float g, float b, float a);

    static Mesh makeTriangularPrism(float width, float height, float depth,
                                    float r, float g, float b, float a);

    static Mesh makeCone(float radius, float height, int sectors,
                         float r, float g, float b, float a);

    static Mesh makeCylinder(float radius, float height, int sectors,
                              float r, float g, float b, float a);

    static Mesh makeHill(float topWidth,    float topDepth,
                         float bottomWidth, float bottomDepth,
                         float height,
                         float r, float g, float b, float a);

    // =========================================================================
    // NEW — GOLF-COURSE FACTORY METHODS  (moved from main.cpp)
    // =========================================================================

// ── makeHole ──────────────────────────────────────────────────────────────────
// Unchanged — returns just the fairway slab at the origin.
//
static Mesh makeHole();
// ── buildHoleMeshes ───────────────────────────────────────────────────────────
// Builds the fairway (via makeHole) + 4 border strips and pushes all of them
// into the caller's mesh array. All meshes sit at the origin; the caller
// translates the whole group into world space.
//
// Fairway:  28 (X) × 0.2 (Y) × 15 (Z)
// Borders:  thickness 0.5, height 0.4 (slightly taller → reads as a kerb)
//           long sides run along X, short sides run along Z
//           short sides are widened by 2×BT so corners are fully covered
//
static void buildHoleMeshes(float rotY, Mesh** out, int& count, int maxCount)
{
    const float FW = 28.0f;   // fairway length (X)
    //const float FH =  0.2f;   // fairway height (Y)  — kept for offset math
    const float FD = 15.0f;   // fairway depth  (Z)

    const float BT =  0.5f;   // border thickness
    const float BH =  0.4f;   // border height (kerb)

    const float BR = 0.9f, BG = 0.9f, BB = 0.9f;   // off-white / concrete

    // ── Fairway ───────────────────────────────────────────────────────────────
    Mesh* fairway = new Mesh(Mesh::makeHole());
    if (rotY != 0.0f) fairway->rotate(rotY, 0.0f, 1.0f, 0.0f);
    if (count < maxCount) out[count++] = fairway;

    // ── Long borders (parallel to X axis, one on each Z side) ────────────────
    Mesh* borderZPos = new Mesh(Mesh::makeCuboid(FW, BH, BT, BR, BG, BB, 1.0f));
    borderZPos->translate(0.0f, 0.0f, (FD + BT) * 0.5f);
    if (rotY != 0.0f) borderZPos->rotate(rotY, 0.0f, 1.0f, 0.0f);
    borderZPos->setup();
    if (count < maxCount) out[count++] = borderZPos;

    Mesh* borderZNeg = new Mesh(Mesh::makeCuboid(FW, BH, BT, BR, BG, BB, 1.0f));
    borderZNeg->translate(0.0f, 0.0f, -(FD + BT) * 0.5f);
    if (rotY != 0.0f) borderZNeg->rotate(rotY, 0.0f, 1.0f, 0.0f);
    borderZNeg->setup();
    if (count < maxCount) out[count++] = borderZNeg;

    // ── Short borders (parallel to Z axis, one on each X side) ───────────────
    // Extended by BT on each Z end so corners are fully covered.
    Mesh* borderXPos = new Mesh(Mesh::makeCuboid(BT, BH, FD + BT * 2.0f,
                                                 BR, BG, BB, 1.0f));
    borderXPos->translate((FW + BT) * 0.5f, 0.0f, 0.0f);
    if (rotY != 0.0f) borderXPos->rotate(rotY, 0.0f, 1.0f, 0.0f);
    borderXPos->setup();
    if (count < maxCount) out[count++] = borderXPos;

    Mesh* borderXNeg = new Mesh(Mesh::makeCuboid(BT, BH, FD + BT * 2.0f,
                                                 BR, BG, BB, 1.0f));
    borderXNeg->translate(-(FW + BT) * 0.5f, 0.0f, 0.0f);
    if (rotY != 0.0f) borderXNeg->rotate(rotY, 0.0f, 1.0f, 0.0f);
    borderXNeg->setup();
    if (count < maxCount) out[count++] = borderXNeg;
}

    // ── Connector strip ───────────────────────────────────────────────────────
    // Thin red cuboid of size (lenX, 0.2, lenZ) centred at (cx, cy, cz).
    // Use lenX or lenZ = small value to make the strip point in a direction.
    static Mesh makeConnector(float cx, float cy, float cz,
                              float lenX, float lenZ);

    // ── Elevation ramp ────────────────────────────────────────────────────────
    // NEW — connects two fairways at different heights.
    // (x0,y0,z0) = centre of the low end,  (x1,y1,z1) = centre of the high end.
    // width = ramp cross-section width (perpendicular to travel direction).
    // The ramp is a cuboid rotated to slope between the two Y values; its
    // length is the 3-D distance between the two end-centres and its height
    // is 0.2 (same as a fairway) before rotation.
    static Mesh makeRamp(float x0, float y0, float z0,
                         float x1, float y1, float z1,
                         float width = 8.0f);

    // ── Tree ─────────────────────────────────────────────────────────────────
    // NEW — pushes a trunk cylinder and canopy cone into `out`.
    // Replaces the free addTree() that was in main.cpp.
    static void addTreeTo(std::vector<Mesh>& out,
                          float x, float z, float scale = 1.0f);

                          // ── makeRock ──────────────────────────────────────────────────────────────────
// A rocky obstacle built from makeHill with asymmetric top/bottom proportions
// and grey colouring to read as a boulder. Built at the origin — caller
// translates into world space.
// scale: overall size multiplier (1.0 = ~3 wide, ~2 tall, ~2.5 deep)
static Mesh makeRock(float scale = 1.0f);

// These are general-purpose terrain / course utilities that any hole can use.
// All methods are static (no instance required), follow the same RGBA colour
// convention as existing factories, and call setup() internally before
// returning, matching the pattern of makeCylinder / makeCone etc.
// =============================================================================

// ── makeArcFairway ────────────────────────────────────────────────────────────
// Subdivided arc-ribbon surface with per-vertex height control.
//
// innerR / outerR   : radii bounding the ribbon.
// startAngle / endAngle : sweep in degrees (CCW from +X in XZ plane).
// ringSegs / radialSegs : angular and radial subdivisions.
// heightFn(arcT, radialT) → float : called for every vertex.
//   arcT    [0,1] = position along arc.
//   radialT [0,1] = position across the band (0=inner, 1=outer).
//   Return 0 for a flat infill copy; return a sinusoidal function for hills.
// r,g,b,a : surface colour.
//
// Normals are computed analytically from the surface partial derivatives, so
// shading is smooth across the undulating surface.
// UVs: u = arcT * uScale, v = radialT * vScale.
//
static Mesh makeArcFairway(float innerR, float outerR,
                           float startAngle, float endAngle,
                           int ringSegs, int radialSegs,
                           const std::function<float(float,float)>& heightFn,
                           float r, float g, float b, float a = 1.0f,
                           float uScale = 4.0f, float vScale = 2.0f);

// ── makeArcBorder ─────────────────────────────────────────────────────────────
// Full 3-D paver kerb following an arc edge. All paver segments are merged into
// one Mesh (one VAO) for efficiency.
//
// radius       : arc radius at which the border edge sits.
// paverWidth   : radial depth of each paver slab.
// paverRaise   : how far the paver top rises above the fairway surface.
// startAngle / endAngle : must match the arc they border (degrees).
// segs         : angular subdivision count.
// side         : +1 = outer (extrudes outward), -1 = inner.
// heightFn     : same contract as makeArcFairway. The paver bottom is always
//                Y=0; the top is heightFn(t,edge) + paverRaise.
//                Pass flat lambda for ground-level infill pavers.
// r,g,b,a      : paver colour.
//
static Mesh makeArcBorder(float radius, float paverWidth,
                          float paverRaise,
                          float startAngle, float endAngle,
                          int segs, int side,
                          const std::function<float(float,float)>& heightFn,
                          float r = 0.76f, float g = 0.74f,
                          float b = 0.70f, float a = 1.0f);

// ── makeHeightmapTerrain ──────────────────────────────────────────────────────
// General flat-grid terrain mesh driven by an arbitrary height function.
// Useful for the surrounding ground, volcano flanks, sand bunkers, etc.
//
// width / depth  : total XZ extent of the grid, centred at origin.
// segsX / segsZ  : vertex subdivisions in each direction.
// heightFn(nx,nz) → float : called per vertex, nx/nz in [0,1].
// r,g,b,a        : surface colour.
//
// Normals are computed per-quad (finite-difference approximation using
// neighbouring vertices), giving correct shading over arbitrary terrain.
//
static Mesh makeHeightmapTerrain(float width, float depth,
                                 int segsX, int segsZ,
                                 const std::function<float(float,float)>& heightFn,
                                 float r, float g, float b, float a = 1.0f);

// ── makeArcWall ───────────────────────────────────────────────────────────────
// Vertical retaining wall following a circular arc. Useful for the inner face
// of raised island walls, bowl walls, or sunken sand bunker sides.
//
// radius      : wall centreline radius.
// wallHeight  : total height of the wall.
// startAngle / endAngle : arc sweep in degrees.
// segs        : angular subdivision count.
// bottomY     : Y of the base of the wall (default 0 = ground).
// topY        : Y of the top of the wall (= bottomY + wallHeight).
//               The wall's top edge can be straight (uniform topY) or
//               driven by topHeightFn(arcT) → float for a stepped profile.
// r,g,b,a     : wall colour.
//
static Mesh makeArcWall(float radius, float wallHeight,
                        float startAngle, float endAngle,
                        int segs,
                        float bottomY,
                        float r, float g, float b, float a = 1.0f);

// ── makeAnnularDisc ───────────────────────────────────────────────────────────
// Flat ring surface (annulus) — useful for water moat surfaces, path rings,
// or the gap fill between an island and the surrounding fairway.
//
// innerR / outerR : ring radii.
// y               : flat Y position of the disc.
// segs            : angular subdivision count.
// r,g,b,a         : surface colour.
//
static Mesh makeAnnularDisc(float innerR, float outerR,
                            float y, int segs,
                            float r, float g, float b, float a = 1.0f);

// ── makeSlopedStrip ───────────────────────────────────────────────────────────
// A subdivided quad strip that rises linearly from one end to the other —
// a general-purpose ramp / incline with smooth shading.
// More controllable than makeRamp (which uses a pre-built cuboid) because
// the top and bottom edges are independently positioned.
//
// x0,y0,z0  : centre of the low end.
// x1,y1,z1  : centre of the high end.
// width      : perpendicular width of the strip.
// segs       : longitudinal subdivisions (use >= 4 for smooth normal lighting).
// r,g,b,a    : strip colour.
//
static Mesh makeSlopedStrip(float x0, float y0, float z0,
                            float x1, float y1, float z1,
                            float width, int segs,
                            float r, float g, float b, float a = 1.0f);

// ── makePathStrip ─────────────────────────────────────────────────────────────
// A flat quad strip following a cubic Bezier centreline.
// Suitable for connector paths, cart tracks, or curved walkways.
//
// p0..p3  : XZ Bezier control points (Y is fixed at pathY).
// width   : strip width perpendicular to the curve tangent.
// segs    : number of quad segments approximating the curve.
// pathY   : height of the strip.
// r,g,b,a : strip colour.
//
static Mesh makePathStrip(glm::vec2 p0, glm::vec2 p1,
                          glm::vec2 p2, glm::vec2 p3,
                          float width, int segs, float pathY,
                          float r, float g, float b, float a = 1.0f);

// ── makeSteppedTerrain ────────────────────────────────────────────────────────
// A series of flat horizontal platforms connected by vertical risers,
// producing a stepped hillside. Useful for volcano terracing or retaining walls
// that separate different elevation zones.
//
// x, z          : world-space XZ centre of the stepped formation.
// stepsX        : number of steps in the X direction.
// stepsZ        : number of steps in the Z direction.
// stepWidth     : XZ extent of each step tread.
// stepHeight    : vertical rise per step.
// riserDepth    : depth of the vertical riser face.
// r,g,b,a       : step colour (riser uses a slightly darkened version).
//
// Pushes all step treads and risers into `out` as separate meshes.
static void makeSteppedTerrain(std::vector<Mesh>& out,
                               float x, float z,
                               int stepsX, int stepsZ,
                               float stepWidth, float stepHeight,
                               float riserDepth,
                               float r, float g, float b, float a = 1.0f);

                               
    static Mesh makeTunnel(float outerRadius, float innerRadius,
                               float height, int sectors,
                               float r, float g, float b, float a);

    static Mesh makeCylinderNoTop(float radius, float height, int sectors,
                               float r, float g, float b, float a);

    static Mesh makeRamp2(float x0, float y0, float z0,
                      float x1, float y1, float z1,
                      float width);
};