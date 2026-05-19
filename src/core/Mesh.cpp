#include "Mesh.h"
#include <cmath>

Mesh::Mesh(float r, float g, float b, float a)
    : r(r), g(g), b(b), a(a),
      modelMatrix(glm::mat4(1.0f)),
      select(false),
      VAO(0), VBO(0), EBO(0),
      wireVAO(0), wireVBO(0),
      buffersInitialized(false),
      vertexCount(0),
      indexCount(0),
      wireVertexCount(0)
{
}

Mesh::~Mesh()
{
    cleanup();
}

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
        vertexCount * sizeof(Vertex),
        vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indexCount * sizeof(unsigned int),
        indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    // texCoord
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

        wireVertices[wireVertexCount++] = vertices[i0].position.x;
        wireVertices[wireVertexCount++] = vertices[i0].position.y;
        wireVertices[wireVertexCount++] = vertices[i0].position.z;
        wireVertices[wireVertexCount++] = vertices[i1].position.x;
        wireVertices[wireVertexCount++] = vertices[i1].position.y;
        wireVertices[wireVertexCount++] = vertices[i1].position.z;

        wireVertices[wireVertexCount++] = vertices[i1].position.x;
        wireVertices[wireVertexCount++] = vertices[i1].position.y;
        wireVertices[wireVertexCount++] = vertices[i1].position.z;
        wireVertices[wireVertexCount++] = vertices[i2].position.x;
        wireVertices[wireVertexCount++] = vertices[i2].position.y;
        wireVertices[wireVertexCount++] = vertices[i2].position.z;

        wireVertices[wireVertexCount++] = vertices[i2].position.x;
        wireVertices[wireVertexCount++] = vertices[i2].position.y;
        wireVertices[wireVertexCount++] = vertices[i2].position.z;
        wireVertices[wireVertexCount++] = vertices[i0].position.x;
        wireVertices[wireVertexCount++] = vertices[i0].position.y;
        wireVertices[wireVertexCount++] = vertices[i0].position.z;

        if (wireVertexCount >= MAX_WIRE - 18)
            break;
    }

    if (wireVertexCount == 0)
        return;

    glGenVertexArrays(1, &wireVAO);
    glGenBuffers(1, &wireVBO);

    glBindVertexArray(wireVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
    glBufferData(GL_ARRAY_BUFFER,
        wireVertexCount * sizeof(float),
        wireVertices, GL_STATIC_DRAW);

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

    GLint colourLocation = glGetUniformLocation(shaderProgram, "colour");
    GLint modelLocation  = glGetUniformLocation(shaderProgram, "model");

    glUseProgram(shaderProgram);

    if (select)
    {
        glUniform4f(colourLocation,
            r * 0.7f + 0.3f,
            g * 0.7f + 0.3f,
            b * 0.7f + 0.3f, a);
    }
    else
    {
        glUniform4f(colourLocation, r, g, b, a);
    }

    // glm::value_ptr gives a column-major float* — use GL_FALSE for transpose
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

void Mesh::resetTransform()
{
    modelMatrix = glm::mat4(1.0f);
}

void Mesh::applyMatrix(const glm::mat4& m)
{
    modelMatrix = m * modelMatrix;
}

void Mesh::setRGBA(float r, float g, float b, float a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

void Mesh::selected(bool s)
{
    select = s;
}

bool Mesh::isSelected()
{
    return select;
}


Mesh Mesh::makeCuboid(float width, float height, float depth,
                      float r, float g, float b, float a)
{
    float w = width  / 2.0f;
    float h = height / 2.0f;
    float d = depth  / 2.0f;

    Mesh mesh(r, g, b, a);

    Vertex verts[] = {
        // Front
        {{-w,-h, d},{0,0,1},{0,0}}, {{ w,-h, d},{0,0,1},{1,0}},
        {{ w, h, d},{0,0,1},{1,1}}, {{-w, h, d},{0,0,1},{0,1}},
        // Back
        {{ w,-h,-d},{0,0,-1},{0,0}}, {{-w,-h,-d},{0,0,-1},{1,0}},
        {{-w, h,-d},{0,0,-1},{1,1}}, {{ w, h,-d},{0,0,-1},{0,1}},
        // Left
        {{-w,-h,-d},{-1,0,0},{0,0}}, {{-w,-h, d},{-1,0,0},{1,0}},
        {{-w, h, d},{-1,0,0},{1,1}}, {{-w, h,-d},{-1,0,0},{0,1}},
        // Right
        {{ w,-h, d},{1,0,0},{0,0}}, {{ w,-h,-d},{1,0,0},{1,0}},
        {{ w, h,-d},{1,0,0},{1,1}}, {{ w, h, d},{1,0,0},{0,1}},
        // Top
        {{-w, h, d},{0,1,0},{0,0}}, {{ w, h, d},{0,1,0},{1,0}},
        {{ w, h,-d},{0,1,0},{1,1}}, {{-w, h,-d},{0,1,0},{0,1}},
        // Bottom
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
    mesh.addTriangle(6, 7, 8);  mesh.addTriangle(8, 9, 6);
    mesh.addTriangle(10,11,12); mesh.addTriangle(12,13,10);
    mesh.addTriangle(14,15,16); mesh.addTriangle(16,17,14);

    return mesh;
}

Mesh Mesh::makeCone(float radius, float height, int sectors,
                    float r, float g, float b, float a)
{
    Mesh mesh(r, g, b, a);

    float h = height / 2.0f;
    glm::vec3 apex      = { 0,  h, 0};
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
        mesh.addVertex({topC,        nUp, {0.5f, 0.5f}});
        mesh.addVertex({{x1,h,z1},   nUp, {0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)}});
        mesh.addVertex({{x2,h,z2},   nUp, {0.5f+0.5f*cosf(a2), 0.5f+0.5f*sinf(a2)}});
        mesh.addTriangle(tBaseIdx, tBaseIdx+1, tBaseIdx+2);

        glm::vec3 nDn = {0,-1,0};
        int bBaseIdx = mesh.vertexCount;
        mesh.addVertex({botC,        nDn, {0.5f, 0.5f}});
        mesh.addVertex({{x2,-h,z2},  nDn, {0.5f+0.5f*cosf(a2), 0.5f+0.5f*sinf(a2)}});
        mesh.addVertex({{x1,-h,z1},  nDn, {0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)}});
        mesh.addTriangle(bBaseIdx, bBaseIdx+1, bBaseIdx+2);
    }

    return mesh;
}