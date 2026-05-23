#pragma once
// Mesh.h  —  full header (existing content preserved; new additions marked NEW)
//
// All object-generating helpers that were previously free functions in main.cpp
// are now static factory methods here so that every shape lives in Mesh.

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

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
    const float FH =  0.2f;   // fairway height (Y)  — kept for offset math
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

static Mesh makeTunnel(float outerRadius, float innerRadius,
                               float height, int sectors,
                               float r, float g, float b, float a);

static Mesh makeCylinderNoTop(float radius, float height, int sectors,
                               float r, float g, float b, float a);
};

