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

    // No copying — two Mesh objects must never share heap arrays or GPU
    // buffer IDs, and a memberwise copy would do exactly that.
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;


    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;


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


    int getVertexCount() const { return vertexCount; }
    int getIndexCount()  const { return indexCount;  }


    static Mesh makeCuboid(float width, float height, float depth,
                           float r, float g, float b, float a = 1.0f);

    static Mesh makeTriangularPrism(float width, float height, float depth,
                                    float r, float g, float b, float a = 1.0f);

    static Mesh makeCone(float radius, float height, int sectors,
                         float r, float g, float b, float a = 1.0f);

    static Mesh makeCylinder(float radius, float height, int sectors,
                              float r, float g, float b, float a = 1.0f);

private:
    // Maximum element counts — kept as constants so limits are documented
    // in one place, but the storage itself lives on the heap.
    static const int MAX_VERTICES = 65536;
    static const int MAX_INDICES  = 65536;
    static const int MAX_WIRE     = 65536 * 6;  // 3 floats × 2 verts × MAX_INDICES triangles

    // Heap-allocated arrays — owned exclusively by this object.
    // Allocated in the constructor, freed in cleanup() / destructor.
    Vertex*       vertices;
    unsigned int* indices;
    float*        wireVertices;

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