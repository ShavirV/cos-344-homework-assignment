#ifndef HOLE13_H
#define HOLE13_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declare GltfMesh to avoid pulling in GltfLoader.h here
namespace gltfloader { struct GltfMesh; }

// =============================================================================
// Hole 13 — "Volcanic Gateway"  (COS344 Practical, 2026)
// =============================================================================
class Hole13
{
public:
    Hole13();
    ~Hole13();

    void init(GLuint shaderProgram,
              const char* glbPath = "volcano.glb");

    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& lightDir,
              const glm::vec3& viewPos) const;

    void cleanup();

    glm::vec3 cupPosition()  const { return _cupPos;      }
    glm::vec3 teePosition()  const { return _teePos;      }
    glm::vec3 sceneCenter()  const { return _sceneCenter; }
    float     sceneRadius()  const { return _sceneRadius; }

private:
    void build();
    void drawProcedural(const glm::mat4& mvp,
                        const glm::mat4& model,
                        const glm::mat3& nrm,
                        const glm::vec3& lightDir,
                        const glm::vec3& viewPos) const;
    void drawVolcano(const glm::mat4& view,
                     const glm::mat4& proj,
                     const glm::vec3& lightDir,
                     const glm::vec3& viewPos) const;

    // Procedural mesh
    GLuint  _vao, _vbo, _ebo;
    GLsizei _indexCount;

    // Lava pool mesh (separate VAO so we can toggle uEmissive)
    GLuint  _lavaVao, _lavaVbo, _lavaEbo;
    GLsizei _lavaIndexCount;

    // Volcano GLB asset
    gltfloader::GltfMesh* _volcano;

    // Shader + uniform locations
    GLuint _program;
    GLint  _uMVP, _uModel, _uNormalMat;
    GLint  _uLightDir, _uLightColor, _uAmbient, _uViewPos;
    GLint  _uEmissive, _uEmissiveColor;
    GLint  _uLavaLightPos, _uLavaLightColor, _uLavaLightIntensity;

    // Scene anchors
    glm::vec3 _teePos, _cupPos, _sceneCenter;
    glm::vec3 _lavaPoolCenter;
    float     _sceneRadius;
};

#endif // HOLE13_H