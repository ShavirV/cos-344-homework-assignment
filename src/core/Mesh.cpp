#include "Mesh.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

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