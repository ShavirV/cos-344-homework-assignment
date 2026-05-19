#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh {
public:
    Mesh(float r, float g, float b, float a = 1.0f);
    ~Mesh();

    void addVertex(const Vertex& v);
    void addIndex(unsigned int i);
    void addTriangle(unsigned int i0, unsigned int i1, unsigned int i2);

    void setup();
    void draw(GLuint shaderProgram, bool wireframe = false);
    void cleanup();

    void translate(float x, float y, float z);
    void rotate(float angle, float ax, float ay, float az);
    void scale(float sx, float sy, float sz);
    void resetTransform();
    void applyMatrix(const glm::mat4& m);

    void setRGBA(float r, float g, float b, float a);
    void selected(bool s);
    bool isSelected();

    // Static factory methods
    static Mesh makeCuboid(float width, float height, float depth,
                           float r, float g, float b, float a = 1.0f);
    static Mesh makeTriangularPrism(float width, float height, float depth,
                                    float r, float g, float b, float a = 1.0f);
    static Mesh makeCone(float radius, float height, int sectors,
                         float r, float g, float b, float a = 1.0f);
    static Mesh makeCylinder(float radius, float height, int sectors,
                              float r, float g, float b, float a = 1.0f);

    int getVertexCount() const { return vertexCount; }
    int getIndexCount()  const { return indexCount;  }


private:
    // Vertex and index storage (plain C-style, no std::vector)
    static const int MAX_VERTICES = 65536;
    static const int MAX_INDICES  = 65536;
    static const int MAX_WIRE     = 65536 * 6;

    Vertex       vertices[MAX_VERTICES];
    unsigned int indices[MAX_INDICES];
    float        wireVertices[MAX_WIRE];

    int vertexCount;
    int indexCount;
    int wireVertexCount;

    float r, g, b, a;
    bool  select;

    glm::mat4 modelMatrix;

    GLuint VAO, VBO, EBO;
    GLuint wireVAO, wireVBO;
    bool   buffersInitialized;

    void buildWireVBO();
};