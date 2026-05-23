#include "Mesh.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <functional> 

// ─────────────────────────────────────────────────────────────────────────────
// Life-cycle
// ─────────────────────────────────────────────────────────────────────────────

Mesh::Mesh(float r, float g, float b, float a)
    : r(r), g(g), b(b), a(a),
      modelMatrix(glm::mat4(1.0f)),
      select(false),
      VAO(0), VBO(0), EBO(0),
      wireVAO(0), wireVBO(0),
      buffersInitialized(false),
      vertexCount(0),
      indexCount(0),
      wireVertexCount(0),
      vertices    (new Vertex      [MAX_VERTICES]),
      indices     (new unsigned int[MAX_INDICES ]),
      wireVertices(new float       [MAX_WIRE    ])
{
}

Mesh::~Mesh()
{
    cleanup();
    delete[] vertices;
    delete[] indices;
    delete[] wireVertices;
}

Mesh::Mesh(Mesh&& other) noexcept
    : r(other.r), g(other.g), b(other.b), a(other.a),
      modelMatrix        (other.modelMatrix),
      select             (other.select),
      VAO                (other.VAO),
      VBO                (other.VBO),
      EBO                (other.EBO),
      wireVAO            (other.wireVAO),
      wireVBO            (other.wireVBO),
      buffersInitialized (other.buffersInitialized),
      vertexCount        (other.vertexCount),
      indexCount         (other.indexCount),
      wireVertexCount    (other.wireVertexCount),
      vertices           (other.vertices),
      indices            (other.indices),
      wireVertices       (other.wireVertices)
{
    other.vertices       = nullptr;
    other.indices        = nullptr;
    other.wireVertices   = nullptr;
    other.vertexCount    = 0;
    other.indexCount     = 0;
    other.wireVertexCount= 0;
    other.VAO = other.VBO = other.EBO = 0;
    other.wireVAO = other.wireVBO = 0;
    other.buffersInitialized = false;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this == &other) return *this;

    cleanup();
    delete[] vertices;
    delete[] indices;
    delete[] wireVertices;

    r = other.r;  g = other.g;  b = other.b;  a = other.a;
    modelMatrix        = other.modelMatrix;
    select             = other.select;
    VAO                = other.VAO;
    VBO                = other.VBO;
    EBO                = other.EBO;
    wireVAO            = other.wireVAO;
    wireVBO            = other.wireVBO;
    buffersInitialized = other.buffersInitialized;
    vertexCount        = other.vertexCount;
    indexCount         = other.indexCount;
    wireVertexCount    = other.wireVertexCount;
    vertices           = other.vertices;
    indices            = other.indices;
    wireVertices       = other.wireVertices;

    other.vertices       = nullptr;
    other.indices        = nullptr;
    other.wireVertices   = nullptr;
    other.vertexCount    = 0;
    other.indexCount     = 0;
    other.wireVertexCount= 0;
    other.VAO = other.VBO = other.EBO = 0;
    other.wireVAO = other.wireVBO = 0;
    other.buffersInitialized = false;

    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Geometry helpers
// ─────────────────────────────────────────────────────────────────────────────

void Mesh::addVertex(const Vertex& v)
{
    if (vertexCount < MAX_VERTICES)
        vertices[vertexCount++] = v;
}

void Mesh::addIndex(unsigned int i)
{
    if (indexCount < MAX_INDICES)
        indices[indexCount++] = i;
}

void Mesh::addTriangle(unsigned int i0, unsigned int i1, unsigned int i2)
{
    addIndex(i0);
    addIndex(i1);
    addIndex(i2);
}

// ─────────────────────────────────────────────────────────────────────────────
// GPU operations
// ─────────────────────────────────────────────────────────────────────────────

void Mesh::cleanup()
{
    if (buffersInitialized)
    {
        if (VAO)     glDeleteVertexArrays(1, &VAO);
        if (VBO)     glDeleteBuffers(1, &VBO);
        if (EBO)     glDeleteBuffers(1, &EBO);
        if (wireVAO) glDeleteVertexArrays(1, &wireVAO);
        if (wireVBO) glDeleteBuffers(1, &wireVBO);

        VAO = VBO = EBO = wireVAO = wireVBO = 0;
        buffersInitialized = false;
    }
}

void Mesh::setup()
{
    if (vertexCount == 0 || indexCount == 0)
        return;

    cleanup();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    // position  — attribute 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // normal    — attribute 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    // texCoord  — attribute 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)(sizeof(glm::vec3) * 2));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    buffersInitialized = true;
    buildWireVBO();
}

void Mesh::buildWireVBO()
{
    wireVertexCount = 0;

    for (int i = 0; i + 2 < indexCount; i += 3)
    {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

#define PUSH_POS(idx) \
        wireVertices[wireVertexCount++] = vertices[idx].position.x; \
        wireVertices[wireVertexCount++] = vertices[idx].position.y; \
        wireVertices[wireVertexCount++] = vertices[idx].position.z;

        PUSH_POS(i0) PUSH_POS(i1)
        PUSH_POS(i1) PUSH_POS(i2)
        PUSH_POS(i2) PUSH_POS(i0)
#undef PUSH_POS

        if (wireVertexCount >= MAX_WIRE - 18)
            break;
    }

    if (wireVertexCount == 0) return;

    glGenVertexArrays(1, &wireVAO);
    glGenBuffers(1, &wireVBO);

    glBindVertexArray(wireVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
    glBufferData(GL_ARRAY_BUFFER,
        wireVertexCount * sizeof(float), wireVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Mesh::draw(GLuint shaderProgram, bool wireframe)
{
    if (!buffersInitialized && vertexCount > 0)
        setup();

    if (vertexCount == 0 || indexCount == 0)
        return;

    // Activate the program BEFORE querying or setting any uniforms.
    // glGetUniformLocation and glUniform* both require the target program
    // to be the currently bound one; calling them beforehand returns -1
    // and silently does nothing.
    glUseProgram(shaderProgram);

    GLint colourLocation = glGetUniformLocation(shaderProgram, "colour");
    GLint modelLocation  = glGetUniformLocation(shaderProgram, "model");

    if (colourLocation != -1)
    {
        if (select)
            glUniform4f(colourLocation, r * 0.7f + 0.3f, g * 0.7f + 0.3f,
                                        b * 0.7f + 0.3f, a);
        else
            glUniform4f(colourLocation, r, g, b, a);
    }

    if (modelLocation != -1)
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    if (wireframe)
    {
        if (wireVertexCount > 0)
        {
            glBindVertexArray(wireVAO);
            glDrawArrays(GL_LINES, 0, wireVertexCount / 3);
            glBindVertexArray(0);
        }
    }
    else
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Transform helpers
// ─────────────────────────────────────────────────────────────────────────────

void Mesh::translate(float x, float y, float z)
{
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
}

void Mesh::rotate(float angle, float ax, float ay, float az)
{
    modelMatrix = glm::rotate(modelMatrix,
        glm::radians(angle), glm::vec3(ax, ay, az));
}

void Mesh::scale(float sx, float sy, float sz)
{
    modelMatrix = glm::scale(modelMatrix, glm::vec3(sx, sy, sz));
}

void Mesh::resetTransform()   { modelMatrix = glm::mat4(1.0f); }

void Mesh::applyMatrix(const glm::mat4& m) { modelMatrix = m * modelMatrix; }

void Mesh::setRGBA(float r_, float g_, float b_, float a_)
{
    r = r_;  g = g_;  b = b_;  a = a_;
}

void Mesh::selected(bool s) { select = s; }
bool Mesh::isSelected()     { return select; }

// =============================================================================
// PRIMITIVE FACTORY METHODS  (unchanged from original)
// =============================================================================

Mesh Mesh::makeCuboid(float width, float height, float depth,
                      float r, float g, float b, float a)
{
    float w = width  / 2.0f;
    float h = height / 2.0f;
    float d = depth  / 2.0f;

    Mesh mesh(r, g, b, a);

    Vertex verts[] = {
        // Front  (+Z)
        {{-w,-h, d},{0,0,1},{0,0}}, {{ w,-h, d},{0,0,1},{1,0}},
        {{ w, h, d},{0,0,1},{1,1}}, {{-w, h, d},{0,0,1},{0,1}},
        // Back   (-Z)
        {{ w,-h,-d},{0,0,-1},{0,0}}, {{-w,-h,-d},{0,0,-1},{1,0}},
        {{-w, h,-d},{0,0,-1},{1,1}}, {{ w, h,-d},{0,0,-1},{0,1}},
        // Left   (-X)
        {{-w,-h,-d},{-1,0,0},{0,0}}, {{-w,-h, d},{-1,0,0},{1,0}},
        {{-w, h, d},{-1,0,0},{1,1}}, {{-w, h,-d},{-1,0,0},{0,1}},
        // Right  (+X)
        {{ w,-h, d},{1,0,0},{0,0}}, {{ w,-h,-d},{1,0,0},{1,0}},
        {{ w, h,-d},{1,0,0},{1,1}}, {{ w, h, d},{1,0,0},{0,1}},
        // Top    (+Y)
        {{-w, h, d},{0,1,0},{0,0}}, {{ w, h, d},{0,1,0},{1,0}},
        {{ w, h,-d},{0,1,0},{1,1}}, {{-w, h,-d},{0,1,0},{0,1}},
        // Bottom (-Y)
        {{-w,-h,-d},{0,-1,0},{0,0}}, {{ w,-h,-d},{0,-1,0},{1,0}},
        {{ w,-h, d},{0,-1,0},{1,1}}, {{-w,-h, d},{0,-1,0},{0,1}},
    };

    for (int i = 0; i < 24; i++)
        mesh.addVertex(verts[i]);

    for (unsigned int face = 0; face < 6; face++)
    {
        unsigned int base = face * 4;
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    }

    return mesh;
}

Mesh Mesh::makeTriangularPrism(float width, float height, float depth,
                                float r, float g, float b, float a)
{
    float w = width  / 2.0f;
    float h = height / 2.0f;
    float d = depth  / 2.0f;

    Mesh mesh(r, g, b, a);

    glm::vec3 fT = { 0,  h,  d};
    glm::vec3 fL = {-w, -h,  d};
    glm::vec3 fR = { w, -h,  d};
    glm::vec3 bT = { 0,  h, -d};
    glm::vec3 bL = {-w, -h, -d};
    glm::vec3 bR = { w, -h, -d};

    glm::vec3 nF  = { 0,  0,  1};
    glm::vec3 nB  = { 0,  0, -1};
    glm::vec3 nBt = { 0, -1,  0};

    float len = sqrtf(h*h + w*w);
    glm::vec3 nL = {-h/len,  w/len, 0};
    glm::vec3 nR = { h/len,  w/len, 0};

    Vertex verts[] = {
        {fL, nF,  {0,     0}}, {fR, nF,  {1,   0}}, {fT, nF,  {0.5f, 1}},
        {bR, nB,  {0,     0}}, {bL, nB,  {1,   0}}, {bT, nB,  {0.5f, 1}},
        {fL, nBt, {0,     0}}, {fR, nBt, {1,   0}}, {bR, nBt, {1,    1}}, {bL, nBt, {0, 1}},
        {fL, nL,  {0,     0}}, {bL, nL,  {1,   0}}, {bT, nL,  {1,    1}}, {fT, nL,  {0, 1}},
        {fR, nR,  {0,     0}}, {fT, nR,  {0,   1}}, {bT, nR,  {1,    1}}, {bR, nR,  {1, 0}},
    };

    for (int i = 0; i < 18; i++)
        mesh.addVertex(verts[i]);

    mesh.addTriangle(0, 1, 2);
    mesh.addTriangle(3, 4, 5);
    mesh.addTriangle(6, 7, 8);  mesh.addTriangle(8,  9,  6);
    mesh.addTriangle(10,11,12); mesh.addTriangle(12, 13, 10);
    mesh.addTriangle(14,15,16); mesh.addTriangle(16, 17, 14);

    return mesh;
}

Mesh Mesh::makeCone(float radius, float height, int sectors,
                    float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float h = height / 2.0f;
    glm::vec3 apex       = { 0,  h, 0};
    glm::vec3 baseCentre = { 0, -h, 0};

    float slopeLen = sqrtf(height*height + radius*radius);
    float nY = radius / slopeLen;
    float nR = height / slopeLen;

    for (int i = 0; i < sectors; i++)
    {
        float a1 = (float)i       / sectors * 2.0f * (float)M_PI;
        float a2 = (float)(i + 1) / sectors * 2.0f * (float)M_PI;

        float x1 = radius * cosf(a1), z1 = radius * sinf(a1);
        float x2 = radius * cosf(a2), z2 = radius * sinf(a2);
        float u1 = (float)i / sectors, u2 = (float)(i+1) / sectors;

        glm::vec3 n1   = glm::normalize(glm::vec3(nR*cosf(a1), nY, nR*sinf(a1)));
        glm::vec3 n2   = glm::normalize(glm::vec3(nR*cosf(a2), nY, nR*sinf(a2)));
        glm::vec3 nMid = glm::normalize(n1 + n2);

        int baseIdx = mesh.vertexCount;
        mesh.addVertex({{x1,-h,z1}, n1,   {u1, 0}});
        mesh.addVertex({{x2,-h,z2}, n2,   {u2, 0}});
        mesh.addVertex({apex,       nMid, {(u1+u2)/2.0f, 1}});
        mesh.addTriangle(baseIdx, baseIdx+1, baseIdx+2);

        glm::vec3 nDown = {0,-1,0};
        int bBaseIdx = mesh.vertexCount;
        mesh.addVertex({baseCentre, nDown, {0.5f, 0.5f}});
        mesh.addVertex({{x2,-h,z2}, nDown, {0.5f+0.5f*cosf(a2), 0.5f+0.5f*sinf(a2)}});
        mesh.addVertex({{x1,-h,z1}, nDown, {0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)}});
        mesh.addTriangle(bBaseIdx, bBaseIdx+1, bBaseIdx+2);
    }

    return mesh;
}

Mesh Mesh::makeCylinder(float radius, float height, int sectors,
                        float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float h = height / 2.0f;
    glm::vec3 topC = {0,  h, 0};
    glm::vec3 botC = {0, -h, 0};

    for (int i = 0; i < sectors; i++)
    {
        float a1 = (float)i       / sectors * 2.0f * (float)M_PI;
        float a2 = (float)(i + 1) / sectors * 2.0f * (float)M_PI;

        float x1 = radius*cosf(a1), z1 = radius*sinf(a1);
        float x2 = radius*cosf(a2), z2 = radius*sinf(a2);
        float u1 = (float)i/sectors, u2 = (float)(i+1)/sectors;

        glm::vec3 n1 = glm::normalize(glm::vec3(cosf(a1), 0, sinf(a1)));
        glm::vec3 n2 = glm::normalize(glm::vec3(cosf(a2), 0, sinf(a2)));

        int baseIdx = mesh.vertexCount;
        mesh.addVertex({{x1,-h,z1}, n1, {u1,0}});
        mesh.addVertex({{x2,-h,z2}, n2, {u2,0}});
        mesh.addVertex({{x2, h,z2}, n2, {u2,1}});
        mesh.addVertex({{x1, h,z1}, n1, {u1,1}});
        mesh.addTriangle(baseIdx,   baseIdx+1, baseIdx+2);
        mesh.addTriangle(baseIdx+2, baseIdx+3, baseIdx);

        glm::vec3 nUp = {0,1,0};
        int tBaseIdx = mesh.vertexCount;
        mesh.addVertex({topC,         nUp, {0.5f, 0.5f}});
        mesh.addVertex({{x1, h, z1},  nUp, {0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)}});
        mesh.addVertex({{x2, h, z2},  nUp, {0.5f+0.5f*cosf(a2), 0.5f+0.5f*sinf(a2)}});
        mesh.addTriangle(tBaseIdx, tBaseIdx+1, tBaseIdx+2);

        glm::vec3 nDn = {0,-1,0};
        int bBaseIdx = mesh.vertexCount;
        mesh.addVertex({botC,         nDn, {0.5f, 0.5f}});
        mesh.addVertex({{x2,-h, z2},  nDn, {0.5f+0.5f*cosf(a2), 0.5f+0.5f*sinf(a2)}});
        mesh.addVertex({{x1,-h, z1},  nDn, {0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)}});
        mesh.addTriangle(bBaseIdx, bBaseIdx+1, bBaseIdx+2);
    }

    return mesh;
}

// ── Deterministic rocky noise (unchanged) ─────────────────────────────────────
static float rockRand(int seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9301;
    seed ^= seed << 4;
    seed *= 0x27d4eb2d;
    seed ^= seed >> 15;
    return (float)(seed & 0xFFFF) / 32767.5f - 1.0f;
}

Mesh Mesh::makeHill(float topWidth,    float topDepth,
                    float bottomWidth, float bottomDepth,
                    float height,
                    float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float tw = topWidth    / 2.0f;
    float td = topDepth    / 2.0f;
    float bw = bottomWidth / 2.0f;
    float bd = bottomDepth / 2.0f;
    float h  = height      / 2.0f;

    glm::vec3 tFL = {-tw,  h, -td};
    glm::vec3 tFR = { tw,  h, -td};
    glm::vec3 tBR = { tw,  h,  td};
    glm::vec3 tBL = {-tw,  h,  td};
    glm::vec3 bFL = {-bw, -h, -bd};
    glm::vec3 bFR = { bw, -h, -bd};
    glm::vec3 bBR = { bw, -h,  bd};
    glm::vec3 bBL = {-bw, -h,  bd};

    // Top face
    {
        glm::vec3 n = {0.0f, 1.0f, 0.0f};
        unsigned int base = mesh.vertexCount;
        mesh.addVertex({tFL, n, {0.0f, 0.0f}});
        mesh.addVertex({tFR, n, {1.0f, 0.0f}});
        mesh.addVertex({tBR, n, {1.0f, 1.0f}});
        mesh.addVertex({tBL, n, {0.0f, 1.0f}});
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    }

    // Bottom face
    {
        glm::vec3 n = {0.0f, -1.0f, 0.0f};
        unsigned int base = mesh.vertexCount;
        mesh.addVertex({bFL, n, {0.0f, 0.0f}});
        mesh.addVertex({bFR, n, {1.0f, 0.0f}});
        mesh.addVertex({bBR, n, {1.0f, 1.0f}});
        mesh.addVertex({bBL, n, {0.0f, 1.0f}});
        mesh.addTriangle(base+0, base+2, base+1);
        mesh.addTriangle(base+2, base+0, base+3);
    }

    const int   subdiv = 6;
    const float jitter = height * 0.08f;

    auto addRockySide = [&](glm::vec3 topL, glm::vec3 topR,
                            glm::vec3 botL, glm::vec3 botR,
                            glm::vec3 faceNormal, int seedOffset)
    {
        int rows     = subdiv + 1;
        int cols     = subdiv + 1;
        int startIdx = mesh.vertexCount;

        for (int row = 0; row < rows; row++)
        {
            float t    = (float)row / subdiv;
            glm::vec3 left  = glm::mix(topL, botL, t);
            glm::vec3 right = glm::mix(topR, botR, t);

            for (int col = 0; col < cols; col++)
            {
                float s   = (float)col / subdiv;
                glm::vec3 pos = glm::mix(left, right, s);

                bool onEdge = (row == 0 || row == subdiv ||
                               col == 0 || col == subdiv);
                if (!onEdge)
                {
                    int seed = seedOffset + row * cols + col;
                    float disp = rockRand(seed) * jitter;
                    pos += faceNormal * disp;
                }

                mesh.addVertex({pos, faceNormal, {s, t}});
            }
        }

        for (int row = 0; row < subdiv; row++)
        {
            for (int col = 0; col < subdiv; col++)
            {
                unsigned int tl = startIdx + row       * cols + col;
                unsigned int tr = startIdx + row       * cols + col + 1;
                unsigned int bl = startIdx + (row + 1) * cols + col;
                unsigned int br = startIdx + (row + 1) * cols + col + 1;

                mesh.addTriangle(tl, bl, tr);
                mesh.addTriangle(tr, bl, br);
            }
        }
    };

    glm::vec3 nFront = glm::normalize(glm::vec3( 0.0f,  h - (-h), -(bd - td)));
    glm::vec3 nBack  = glm::normalize(glm::vec3( 0.0f,  h - (-h),  (bd - td)));
    glm::vec3 nLeft  = glm::normalize(glm::vec3(-(bw - tw), h - (-h), 0.0f));
    glm::vec3 nRight = glm::normalize(glm::vec3( (bw - tw), h - (-h), 0.0f));

    addRockySide(tFL, tFR, bFL, bFR, nFront, 1000);
    addRockySide(tBR, tBL, bBR, bBL, nBack,  2000);
    addRockySide(tBL, tFL, bBL, bFL, nLeft,  3000);
    addRockySide(tFR, tBR, bFR, bBR, nRight, 4000);

    return mesh;
}

// =============================================================================
// NEW — GOLF-COURSE FACTORY METHODS
// =============================================================================

// ── makeHole ─────────────────────────────────────────────────────────────────
// A fairway: flat red cuboid 28 × 0.2 × 10, centred at (cx, cy, cz).
// rotY: Y-axis rotation in degrees (0° = length along X, 90° = length along Z).
//
Mesh Mesh::makeHole()
{
    // Fairway colour: bright red so it reads clearly from above.
    Mesh h = Mesh::makeCuboid(28.0f, 0.2f, 15.0f, 1.0f, 0.05f, 0.05f, 1.0f);
    h.setup();
    return h;
}

// ── makeConnector ─────────────────────────────────────────────────────────────
// Narrow strip linking two adjacent holes.
// (cx, cy, cz) = world centre; lenX and lenZ define the footprint dimensions.
//
Mesh Mesh::makeConnector(float cx, float cy, float cz,
                         float lenX, float lenZ)
{
    Mesh c = Mesh::makeCuboid(lenX, 0.2f, lenZ, 0.85f, 0.05f, 0.05f, 1.0f);
    c.translate(cx, cy, cz);
    c.setup();
    return c;
}

// ── makeRamp ─────────────────────────────────────────────────────────────────
// Sloped ramp connecting two points at different Y values.
//
// Strategy
// --------
// 1. Compute the vector from (x0,y0,z0) to (x1,y1,z1).
// 2. The ramp length is the 3-D distance between the two centres.
// 3. We build a flat cuboid of that length, then rotate it about the axis
//    perpendicular to the travel direction in the XZ plane so that it slopes
//    from y0 to y1.  The rotation axis is the world-space side axis of the
//    ramp (perpendicular to travel, lying in the horizontal plane).
// 4. Finally translate to the midpoint.
//
// The ramp top surface normal will be tilted from vertical by the slope angle,
// which looks correct.  Thickness is 0.2 (same as a fairway) before tilt.
//
Mesh Mesh::makeRamp(float x0, float y0, float z0,
                    float x1, float y1, float z1,
                    float width)
{
    float dx  = x1 - x0;
    float dy  = y1 - y0;
    float dz  = z1 - z0;

    float len3D = sqrtf(dx*dx + dy*dy + dz*dz);
    if (len3D < 0.001f) len3D = 0.001f;

    float lenXZ    = sqrtf(dx*dx + dz*dz);
    float slopeRad = atan2f(dy, lenXZ);
    float headingRad = atan2f(dz, dx);

    float w = width / 2.0f;
    float h = dy;        // full height of the wedge
    float d = lenXZ;     // horizontal run

    Mesh mesh(0.20f, 0.45f, 0.10f, 1.0f);  // same green as landmass

    // 8 vertices: 4 at base (y=0), 4 at top-end (y=h)
    // Base rectangle (z = -d/2 .. +d/2, all at y = -h/2)
    glm::vec3 bBL = {-w, -h/2.0f,  d/2.0f};  // base back left
    glm::vec3 bBR = { w, -h/2.0f,  d/2.0f};  // base back right
    glm::vec3 bFL = {-w, -h/2.0f, -d/2.0f};  // base front left
    glm::vec3 bFR = { w, -h/2.0f, -d/2.0f};  // base front right

    // Top edge (only at the high end, z = -d/2)
    glm::vec3 tFL = {-w,  h/2.0f, -d/2.0f};  // top front left
    glm::vec3 tFR = { w,  h/2.0f, -d/2.0f};  // top front right

    // ── Bottom face ───────────────────────────────────────────────────────────
    glm::vec3 nDn = {0, -1, 0};
    unsigned int base = mesh.vertexCount;
    mesh.addVertex({bFL, nDn, {0,0}});
    mesh.addVertex({bFR, nDn, {1,0}});
    mesh.addVertex({bBR, nDn, {1,1}});
    mesh.addVertex({bBL, nDn, {0,1}});
    mesh.addTriangle(base+0, base+2, base+1);
    mesh.addTriangle(base+2, base+0, base+3);

    // ── Slope (top) face ──────────────────────────────────────────────────────
    glm::vec3 nSlope = glm::normalize(glm::vec3(0, d, h));
    base = mesh.vertexCount;
    mesh.addVertex({bBL, nSlope, {0,0}});
    mesh.addVertex({bBR, nSlope, {1,0}});
    mesh.addVertex({tFR, nSlope, {1,1}});
    mesh.addVertex({tFL, nSlope, {0,1}});
    mesh.addTriangle(base+0, base+1, base+2);
    mesh.addTriangle(base+2, base+3, base+0);

    // ── Back face (vertical wall at base end) ─────────────────────────────────
    glm::vec3 nBack = {0, 0, 1};
    base = mesh.vertexCount;
    mesh.addVertex({bBL, nBack, {0,0}});
    mesh.addVertex({bBR, nBack, {1,0}});
    mesh.addVertex({bBR, nBack, {1,0}});  // no height at back
    mesh.addVertex({bBL, nBack, {0,0}});
    // just a degenerate edge — skip, back is open at ground level

    // ── Front face (vertical wall at high end) ────────────────────────────────
    glm::vec3 nFront = {0, 0, -1};
    base = mesh.vertexCount;
    mesh.addVertex({bFL, nFront, {0,0}});
    mesh.addVertex({bFR, nFront, {1,0}});
    mesh.addVertex({tFR, nFront, {1,1}});
    mesh.addVertex({tFL, nFront, {0,1}});
    mesh.addTriangle(base+0, base+1, base+2);
    mesh.addTriangle(base+2, base+3, base+0);

    // ── Left triangular face ──────────────────────────────────────────────────
    glm::vec3 nLeft = {-1, 0, 0};
    base = mesh.vertexCount;
    mesh.addVertex({bBL, nLeft, {0,0}});
    mesh.addVertex({bFL, nLeft, {1,0}});
    mesh.addVertex({tFL, nLeft, {1,1}});
    mesh.addTriangle(base+0, base+1, base+2);

    // ── Right triangular face ─────────────────────────────────────────────────
    glm::vec3 nRight = {1, 0, 0};
    base = mesh.vertexCount;
    mesh.addVertex({bBR, nRight, {0,0}});
    mesh.addVertex({tFR, nRight, {1,1}});
    mesh.addVertex({bFR, nRight, {1,0}});
    mesh.addTriangle(base+0, base+1, base+2);

    // ── Orient and place ──────────────────────────────────────────────────────
    mesh.rotate(glm::degrees(headingRad), 0.0f, 1.0f, 0.0f);
    mesh.translate((x0 + x1) * 0.5f,
                   (y0 + y1) * 0.5f,
                   (z0 + z1) * 0.5f);

    mesh.setup();
    return mesh;
}

// ── addTreeTo ────────────────────────────────────────────────────────────────
// Pushes a trunk (cylinder) and canopy (cone) into `out` at world position
// (x, ?, z), scaling all dimensions by `scale`.
// Replaces the free addTree() helper that was in main.cpp.
//
void Mesh::addTreeTo(std::vector<Mesh>& out, float x, float z, float scale)
{
    Mesh trunk = Mesh::makeCylinder(0.5f * scale, 4.0f * scale, 12,
                                    0.36f, 0.25f, 0.13f, 1.0f);
    trunk.translate(x, 2.0f * scale, z);
    trunk.setup();
    out.push_back(std::move(trunk));

    Mesh canopy = Mesh::makeCone(3.0f * scale, 6.0f * scale, 16,
                                  0.13f, 0.55f, 0.20f, 1.0f);
    canopy.translate(x, 7.0f * scale, z);
    canopy.setup();
    out.push_back(std::move(canopy));
}

// ── makeRock ──────────────────────────────────────────────────────────────────
// Rocky boulder obstacle. Wraps makeHill with:
//   - narrow top, wider base  → looks like a natural boulder
//   - slight asymmetry in depth vs width → non-uniform silhouette
//   - grey colour (0.55, 0.55, 0.55)
//   - the makeHill jitter gives the sides their rough, rocky texture
// Built at the origin; caller is responsible for translate/rotate.
//
Mesh Mesh::makeRock(float scale)
{
    Mesh rock = Mesh::makeHill(
        1.5f * scale,   // topWidth    — narrow top
        1.2f * scale,   // topDepth
        3.0f * scale,   // bottomWidth — wider base
        2.5f * scale,   // bottomDepth
        2.0f * scale,   // height
        0.55f, 0.55f, 0.55f, 1.0f   // grey
    );
    return rock;
}

// =============================================================================
// ADD TO Mesh.cpp — paste after makeRock() at the bottom of the file.
// These are the two factory methods declared in Mesh.h that are called
// internally by Hole16 via its private builder methods.  They are plain
// static free functions here because they depend on fairwayHeight(), which
// is defined at the top of Hole16.cpp and must remain there (it encodes the
// per-hole terrain character).  Hole16 calls them through its own wrappers
// (Hole16::makeArcFairway / Hole16::makeArcBorder) which know the height fn.
//
// If you later want a truly generic arc primitive that other holes can reuse,
// pass the height function as a std::function<float(float,float)> parameter.
// =============================================================================

// ── fairwayHeight — local height function (copy from Hole16.cpp) ─────────────
// arcT   : [0,1] normalised position along the arc
// radialT: [0,1] normalised position across the band (0=inner, 1=outer)
//
// This duplicate exists so that makeArcBorder (which must pin border bottoms
// to the surface) can call it when implemented as a Mesh:: static.  When used
// only from Hole16, the version in Hole16.cpp is sufficient.
//
// NOTE: keep the constants here in sync with the ones at the top of Hole16.cpp.
static float h16_fairwayHeight(float arcT, float radialT)
{
    const float HILL_FREQ_A = 2.5f,  HILL_AMP_A = 0.70f;
    const float HILL_FREQ_B = 4.8f,  HILL_AMP_B = 0.35f;
    const float CROWN_AMP   = 0.25f;

    float h = HILL_AMP_A * sinf(arcT * HILL_FREQ_A * 2.0f * (float)M_PI)
            + HILL_AMP_B * sinf(arcT * HILL_FREQ_B * 2.0f * (float)M_PI + 1.1f);
    float crown = CROWN_AMP * (1.0f - fabsf(radialT * 2.0f - 1.0f));
    return h + crown;
}

// =============================================================================
// ADD TO Mesh.cpp — paste after makeRock() (replacing the old arc stubs).
//
// All implementations match the winding / normal conventions of the existing
// primitives: CCW triangles, normals pointing outward / upward.
// setup() is called before return so the caller never needs to.
// =============================================================================



// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeArcFairway
// ─────────────────────────────────────────────────────────────────────────────
Mesh Mesh::makeArcFairway(float innerR, float outerR,
                          float startAngle, float endAngle,
                          int ringSegs, int radialSegs,
                          const std::function<float(float,float)>& heightFn,
                          float r, float g, float b, float a,
                          float uScale, float vScale)
{
    Mesh mesh(r, g, b, a);

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
            // Analytic surface normal: cross(dPos/dRadius, dPos/dTheta)
            glm::vec3 dRad   = { cosA, 0.0f, sinA };
            glm::vec3 dTheta = { -radius * sinA, 0.0f, radius * cosA };
            glm::vec3 n      = glm::normalize(glm::cross(dRad, dTheta));
            if (n.y < 0.0f) n = -n;

            mesh.addVertex({ pos, n, { arcT * uScale, radialT * vScale } });
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

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeArcBorder
// ─────────────────────────────────────────────────────────────────────────────
// All paver segments are merged into one Mesh — one VAO, one draw call.
// Each segment is a full 6-face oriented cuboid built from the arc's local
// tangent and radial basis vectors, so every face has a correct outward normal.
//
Mesh Mesh::makeArcBorder(float radius, float paverWidth,
                         float paverRaise,
                         float startAngle, float endAngle,
                         int segs, int side,
                         const std::function<float(float,float)>& heightFn,
                         float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float startRad = startAngle * (float)M_PI / 180.0f;
    float endRad   = endAngle   * (float)M_PI / 180.0f;

    // Paver centreline sits side*paverWidth/2 outside the edge radius.
    float paverCentreR = radius + (float)side * paverWidth * 0.5f;

    // Helper: emit one oriented quad into the shared mesh.
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
        float a0   = startRad + (endRad - startRad) * t0;
        float a1   = startRad + (endRad - startRad) * t1;
        float aMid = (a0 + a1) * 0.5f;

        // Terrain-surface Y at this segment's edge position.
        float radialEdge = (side > 0) ? 1.0f : 0.0f;
        float surfY      = heightFn((t0 + t1) * 0.5f, radialEdge);

        // Paver bottom always at Y=0 (ground); top above surface.
        float bottomY = 0.0f;
        float topY    = surfY + paverRaise;
        float paverH  = topY - bottomY;
        if (paverH < 0.05f) paverH = 0.05f;

        // Arc chord length for this segment (with small overlap to close gaps).
        float segArcLen = fabsf(a1 - a0) * paverCentreR * 1.02f;

        // Centre position of this paver in world XZ.
        float cosM = cosf(aMid), sinM = sinf(aMid);
        float cx   = paverCentreR * cosM;
        float cz   = paverCentreR * sinM;
        float cy   = bottomY + paverH * 0.5f;

        // Oriented basis (all unit vectors):
        //   tang   = arc tangent direction (CCW)
        //   radOut = radially outward direction (away from arc centre), sign-flipped for inner
        //   up     = world up
        glm::vec3 tang   = glm::normalize(glm::vec3(-sinM, 0.0f,  cosM));
        glm::vec3 radOut = glm::normalize(glm::vec3( cosM * (float)side, 0.0f, sinM * (float)side));
        glm::vec3 up     = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 centre = glm::vec3(cx, cy, cz);

        float hw = segArcLen     * 0.5f;  // half arc-length
        float hh = paverH        * 0.5f;  // half height
        float hd = paverWidth    * 0.5f;  // half radial depth

        // 8 oriented corners:  c[arcSign][vertSign][depSign]
        //   arcSign  : 0=back (-tang), 1=front (+tang)
        //   vertSign : 0=bottom (-up), 1=top (+up)
        //   depSign  : 0=inner (-radOut), 1=outer (+radOut)
        glm::vec3 c[2][2][2];
        for (int ai = 0; ai < 2; ai++)
        for (int vi = 0; vi < 2; vi++)
        for (int di = 0; di < 2; di++)
        {
            float ts = (ai == 0) ? -hw : hw;
            float vs = (vi == 0) ? -hh : hh;
            float ds = (di == 0) ? -hd : hd;
            c[ai][vi][di] = centre + tang*ts + up*vs + radOut*ds;
        }

        // Emit 6 faces (CCW winding when viewed from outside/top/end):
        emitFace(c[0][1][0], c[1][1][0], c[1][1][1], c[0][1][1],  up);       // top
        emitFace(c[0][0][1], c[1][0][1], c[1][0][0], c[0][0][0], -up);       // bottom
        emitFace(c[0][0][1], c[0][1][1], c[1][1][1], c[1][0][1],  radOut);   // outer radial
        emitFace(c[1][0][0], c[1][1][0], c[0][1][0], c[0][0][0], -radOut);   // inner radial
        emitFace(c[0][0][0], c[0][1][0], c[0][1][1], c[0][0][1], -tang);     // arc-start cap
        emitFace(c[1][0][1], c[1][1][1], c[1][1][0], c[1][0][0],  tang);     // arc-end cap
    }

    mesh.setup();
    return mesh;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeHeightmapTerrain
// ─────────────────────────────────────────────────────────────────────────────
// General-purpose grid terrain. heightFn(nx, nz) with nx,nz in [0,1].
// Normals are computed by finite-difference from neighbouring vertex heights,
// giving smooth shading across curved terrain.
//
Mesh Mesh::makeHeightmapTerrain(float width, float depth,
                                int segsX, int segsZ,
                                const std::function<float(float,float)>& heightFn,
                                float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    int cols = segsX + 1;
    int rows = segsZ + 1;

    // Sample height at a normalised grid position (clamped to edge).
    auto sample = [&](int xi, int zi) -> float {
        float nx = glm::clamp((float)xi / segsX, 0.0f, 1.0f);
        float nz = glm::clamp((float)zi / segsZ, 0.0f, 1.0f);
        return heightFn(nx, nz);
    };

    for (int zi = 0; zi < rows; zi++)
    {
        float nz = (float)zi / segsZ;
        float wz = (nz - 0.5f) * depth;

        for (int xi = 0; xi < cols; xi++)
        {
            float nx = (float)xi / segsX;
            float wx = (nx - 0.5f) * width;
            float wy = heightFn(nx, nz);

            // Finite-difference normal from neighbours.
            float hL = sample(xi - 1, zi);
            float hR = sample(xi + 1, zi);
            float hD = sample(xi, zi - 1);
            float hU = sample(xi, zi + 1);
            float cellW = width  / segsX;
            float cellD = depth  / segsZ;
            glm::vec3 n = glm::normalize(glm::vec3(
                (hL - hR) / (2.0f * cellW),
                1.0f,
                (hD - hU) / (2.0f * cellD)));

            mesh.addVertex({ {wx, wy, wz}, n, {nx, nz} });
        }
    }

    for (int zi = 0; zi < segsZ; zi++)
        for (int xi = 0; xi < segsX; xi++)
        {
            unsigned int tl = (unsigned int)( zi      * cols + xi     );
            unsigned int tr = (unsigned int)( zi      * cols + xi + 1 );
            unsigned int bl = (unsigned int)((zi + 1) * cols + xi     );
            unsigned int br = (unsigned int)((zi + 1) * cols + xi + 1 );
            mesh.addTriangle(tl, bl, tr);
            mesh.addTriangle(tr, bl, br);
        }

    mesh.setup();
    return mesh;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeArcWall
// ─────────────────────────────────────────────────────────────────────────────
// Vertical curved wall. Each segment is a vertical quad, normals pointing
// radially inward (wall faces the inside of the arc).
//
Mesh Mesh::makeArcWall(float radius, float wallHeight,
                       float startAngle, float endAngle,
                       int segs, float bottomY,
                       float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float startRad = startAngle * (float)M_PI / 180.0f;
    float endRad   = endAngle   * (float)M_PI / 180.0f;
    float topY     = bottomY + wallHeight;

    for (int i = 0; i < segs; i++)
    {
        float t0 = (float)i       / segs;
        float t1 = (float)(i + 1) / segs;
        float a0 = startRad + (endRad - startRad) * t0;
        float a1 = startRad + (endRad - startRad) * t1;

        glm::vec3 p00 = { radius*cosf(a0), bottomY, radius*sinf(a0) };
        glm::vec3 p01 = { radius*cosf(a0), topY,    radius*sinf(a0) };
        glm::vec3 p10 = { radius*cosf(a1), bottomY, radius*sinf(a1) };
        glm::vec3 p11 = { radius*cosf(a1), topY,    radius*sinf(a1) };

        // Inward normal (toward arc centre).
        float aMid = (a0 + a1) * 0.5f;
        glm::vec3 n = glm::normalize(glm::vec3(-cosf(aMid), 0.0f, -sinf(aMid)));

        unsigned int base = (unsigned int)mesh.vertexCount;
        mesh.addVertex({ p00, n, { t0, 0.0f } });
        mesh.addVertex({ p10, n, { t1, 0.0f } });
        mesh.addVertex({ p11, n, { t1, 1.0f } });
        mesh.addVertex({ p01, n, { t0, 1.0f } });
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    }

    mesh.setup();
    return mesh;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeAnnularDisc
// ─────────────────────────────────────────────────────────────────────────────
// Flat ring surface. Suitable for water moats, decorative path rings, etc.
//
Mesh Mesh::makeAnnularDisc(float innerR, float outerR,
                           float y, int segs,
                           float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    glm::vec3 up = {0.0f, 1.0f, 0.0f};

    for (int i = 0; i < segs; i++)
    {
        float a0 = (float)i       / segs * 2.0f * (float)M_PI;
        float a1 = (float)(i + 1) / segs * 2.0f * (float)M_PI;
        float t0 = (float)i       / segs;
        float t1 = (float)(i + 1) / segs;

        glm::vec3 innerA = { innerR * cosf(a0), y, innerR * sinf(a0) };
        glm::vec3 innerB = { innerR * cosf(a1), y, innerR * sinf(a1) };
        glm::vec3 outerA = { outerR * cosf(a0), y, outerR * sinf(a0) };
        glm::vec3 outerB = { outerR * cosf(a1), y, outerR * sinf(a1) };

        unsigned int base = (unsigned int)mesh.vertexCount;
        mesh.addVertex({ innerA, up, { 0.0f, t0 } });
        mesh.addVertex({ outerA, up, { 1.0f, t0 } });
        mesh.addVertex({ outerB, up, { 1.0f, t1 } });
        mesh.addVertex({ innerB, up, { 0.0f, t1 } });
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    }

    mesh.setup();
    return mesh;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeSlopedStrip
// ─────────────────────────────────────────────────────────────────────────────
// Subdivided ribbon rising from (x0,y0,z0) to (x1,y1,z1).
// More controllable than makeRamp: vertices are individually placed so normals
// are consistent across the slope, and segs >= 4 gives smooth shading.
//
Mesh Mesh::makeSlopedStrip(float x0, float y0, float z0,
                           float x1, float y1, float z1,
                           float width, int segs,
                           float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    // Travel direction and perpendicular.
    glm::vec3 travel = glm::normalize(glm::vec3(x1-x0, y1-y0, z1-z0));
    glm::vec3 worldUp = {0.0f, 1.0f, 0.0f};
    glm::vec3 perp    = glm::normalize(glm::cross(travel, worldUp));
    glm::vec3 n       = glm::normalize(glm::cross(perp, travel)); // surface normal

    glm::vec3 start = {x0, y0, z0};
    glm::vec3 end   = {x1, y1, z1};

    int rows = segs + 1;

    for (int i = 0; i < rows; i++)
    {
        float t   = (float)i / segs;
        glm::vec3 centre = glm::mix(start, end, t);
        glm::vec3 left   = centre - perp * (width * 0.5f);
        glm::vec3 right  = centre + perp * (width * 0.5f);

        mesh.addVertex({ left,  n, { 0.0f, t } });
        mesh.addVertex({ right, n, { 1.0f, t } });
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

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makePathStrip
// ─────────────────────────────────────────────────────────────────────────────
// Flat Bezier-following quad strip. Useful for curved connectors, cart paths,
// or river banks. Each segment is a quad aligned to the curve tangent.
//
Mesh Mesh::makePathStrip(glm::vec2 p0, glm::vec2 p1,
                         glm::vec2 p2, glm::vec2 p3,
                         float width, int segs, float pathY,
                         float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);
    glm::vec3 up = {0.0f, 1.0f, 0.0f};

    auto evalBez = [&](float t) -> glm::vec2 {
        float u = 1.0f - t;
        return u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
    };
    auto evalTan = [&](float t) -> glm::vec2 {
        float u = 1.0f - t;
        return 3.0f*(u*u*(p1-p0) + 2.0f*u*t*(p2-p1) + t*t*(p3-p2));
    };

    int rows = segs + 1;
    for (int i = 0; i < rows; i++)
    {
        float t   = (float)i / segs;
        glm::vec2 pos = evalBez(t);
        glm::vec2 tan = evalTan(t);
        if (glm::length(tan) < 1e-6f) tan = glm::vec2(1.0f, 0.0f);
        tan = glm::normalize(tan);

        // Perpendicular in XZ (rotate tan 90° CW).
        glm::vec2 perp = { tan.y, -tan.x };

        glm::vec3 left  = { pos.x - perp.x*width*0.5f, pathY, pos.y - perp.y*width*0.5f };
        glm::vec3 right = { pos.x + perp.x*width*0.5f, pathY, pos.y + perp.y*width*0.5f };

        mesh.addVertex({ left,  up, { 0.0f, t } });
        mesh.addVertex({ right, up, { 1.0f, t } });
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

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::makeSteppedTerrain
// ─────────────────────────────────────────────────────────────────────────────
// Pushes step treads (flat cuboids) and risers (vertical cuboids) into `out`.
// Steps ascend in +Y in the +X direction; use translate/rotate to orient.
//
void Mesh::makeSteppedTerrain(std::vector<Mesh>& out,
                              float x, float z,
                              int stepsX, int stepsZ,
                              float stepWidth, float stepHeight,
                              float riserDepth,
                              float r, float g, float b, float a)
{
    for (int sx = 0; sx < stepsX; sx++)
    {
        float stepY  = (float)(sx + 1) * stepHeight;
        float stepX  = x + (float)sx * stepWidth + stepWidth * 0.5f;
        float totalZ = (float)stepsZ * stepWidth;

        // Tread — flat top surface of the step.
        Mesh tread = Mesh::makeCuboid(
            stepWidth, 0.10f, totalZ,
            r, g, b, a);
        tread.translate(stepX, stepY, z);
        tread.setup();
        out.push_back(std::move(tread));

        // Riser — vertical face connecting this step to the one below.
        // Slightly darkened to read as shadow/depth.
        Mesh riser = Mesh::makeCuboid(
            riserDepth, stepHeight, totalZ,
            r * 0.72f, g * 0.72f, b * 0.72f, a);
        riser.translate(x + (float)sx * stepWidth + riserDepth * 0.5f,
                        stepY - stepHeight * 0.5f,
                        z);
        riser.setup();
        out.push_back(std::move(riser));
    }
}

Mesh Mesh::makeTunnel(float outerRadius, float innerRadius,
                               float height, int sectors,
                               float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);
    float h = height / 2.0f;

    for (int i = 0; i < sectors; i++)
    {
        float a1 = (float)i       / sectors * 2.0f * (float)M_PI;
        float a2 = (float)(i + 1) / sectors * 2.0f * (float)M_PI;

        float ox1 = outerRadius * cosf(a1), oz1 = outerRadius * sinf(a1);
        float ox2 = outerRadius * cosf(a2), oz2 = outerRadius * sinf(a2);
        float ix1 = innerRadius * cosf(a1), iz1 = innerRadius * sinf(a1);
        float ix2 = innerRadius * cosf(a2), iz2 = innerRadius * sinf(a2);

        glm::vec3 nOut1 = glm::normalize(glm::vec3(cosf(a1), 0, sinf(a1)));
        glm::vec3 nOut2 = glm::normalize(glm::vec3(cosf(a2), 0, sinf(a2)));
        glm::vec3 nIn1  = -nOut1;
        glm::vec3 nIn2  = -nOut2;

        // ── Outer wall ────────────────────────────────────────────────────────
        unsigned int base = mesh.vertexCount;
        mesh.addVertex({{ox1, -h, oz1}, nOut1, {(float)i/sectors,     0}});
        mesh.addVertex({{ox2, -h, oz2}, nOut2, {(float)(i+1)/sectors, 0}});
        mesh.addVertex({{ox2,  h, oz2}, nOut2, {(float)(i+1)/sectors, 1}});
        mesh.addVertex({{ox1,  h, oz1}, nOut1, {(float)i/sectors,     1}});
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);

        // ── Inner wall ────────────────────────────────────────────────────────
        base = mesh.vertexCount;
        mesh.addVertex({{ix2, -h, iz2}, nIn2, {(float)(i+1)/sectors, 0}});
        mesh.addVertex({{ix1, -h, iz1}, nIn1, {(float)i/sectors,     0}});
        mesh.addVertex({{ix1,  h, iz1}, nIn1, {(float)i/sectors,     1}});
        mesh.addVertex({{ix2,  h, iz2}, nIn2, {(float)(i+1)/sectors, 1}});
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);

        // ── Top annulus ring ──────────────────────────────────────────────────
        glm::vec3 nUp = {0, 1, 0};
        base = mesh.vertexCount;
        mesh.addVertex({{ox1,  h, oz1}, nUp, {0, 0}});
        mesh.addVertex({{ox2,  h, oz2}, nUp, {1, 0}});
        mesh.addVertex({{ix2,  h, iz2}, nUp, {1, 1}});
        mesh.addVertex({{ix1,  h, iz1}, nUp, {0, 1}});
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);

        // ── Bottom annulus ring ───────────────────────────────────────────────
        glm::vec3 nDn = {0, -1, 0};
        base = mesh.vertexCount;
        mesh.addVertex({{ox2, -h, oz2}, nDn, {1, 0}});
        mesh.addVertex({{ox1, -h, oz1}, nDn, {0, 0}});
        mesh.addVertex({{ix1, -h, iz1}, nDn, {0, 1}});
        mesh.addVertex({{ix2, -h, iz2}, nDn, {1, 1}});
        mesh.addTriangle(base+0, base+1, base+2);
        mesh.addTriangle(base+2, base+3, base+0);
    }

    return mesh;
}

Mesh Mesh::makeCylinderNoTop(float radius, float height, int sectors,
                              float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float h = height / 2.0f;
    glm::vec3 botC = {0, -h, 0};

    for (int i = 0; i < sectors; i++)
    {
        float a1 = (float)i       / sectors * 2.0f * (float)M_PI;
        float a2 = (float)(i + 1) / sectors * 2.0f * (float)M_PI;

        float x1 = radius*cosf(a1), z1 = radius*sinf(a1);
        float x2 = radius*cosf(a2), z2 = radius*sinf(a2);
        float u1 = (float)i/sectors, u2 = (float)(i+1)/sectors;

        glm::vec3 n1 = glm::normalize(glm::vec3(cosf(a1), 0, sinf(a1)));
        glm::vec3 n2 = glm::normalize(glm::vec3(cosf(a2), 0, sinf(a2)));

        // ── Side wall only ────────────────────────────────────────────────────
        int baseIdx = mesh.vertexCount;
        mesh.addVertex({{x1,-h,z1}, n1, {u1,0}});
        mesh.addVertex({{x2,-h,z2}, n2, {u2,0}});
        mesh.addVertex({{x2, h,z2}, n2, {u2,1}});
        mesh.addVertex({{x1, h,z1}, n1, {u1,1}});
        mesh.addTriangle(baseIdx,   baseIdx+1, baseIdx+2);
        mesh.addTriangle(baseIdx+2, baseIdx+3, baseIdx);

        // ── Bottom cap only ───────────────────────────────────────────────────
        glm::vec3 nDn = {0,-1,0};
        int bBaseIdx = mesh.vertexCount;
        mesh.addVertex({botC,         nDn, {0.5f, 0.5f}});
        mesh.addVertex({{x2,-h, z2},  nDn, {0.5f+0.5f*cosf(a2), 0.5f+0.5f*sinf(a2)}});
        mesh.addVertex({{x1,-h, z1},  nDn, {0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)}});
        mesh.addTriangle(bBaseIdx, bBaseIdx+1, bBaseIdx+2);
    }

    return mesh;
}