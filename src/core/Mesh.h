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

    // ── Fairway (one hole) ────────────────────────────────────────────────────
    // Produces a red cuboid 28 × 0.2 × 10 centred at (cx, cy, cz).
    // rotY: Y-axis rotation in degrees — 0° = east-west, 90° = north-south.
    // After construction call setup() yourself (already called inside).
    static Mesh makeHole(float cx, float cy, float cz, float rotY = 0.0f);

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
};