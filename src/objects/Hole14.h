#ifndef HOLE14_H
#define HOLE14_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =============================================================================
// Hole 14 - "Raised Island Green"
//
// Signature Highland Volcanic Zone hole. The report (4.1, 5.3) describes:
//   "The hole's cup is situated on a raised circular island platform
//    reachable through a narrow inclined path. Rendered as a short
//    cylindrical pedestal topped by a flat circular fairway."
//
// Implementation:
//   - Long tee fairway leading toward the pedestal
//   - Narrow inclined ramp bridging tee fairway up to pedestal top
//   - Tall cylindrical basalt pedestal (radius ~12, height ~6)
//   - Annular emissive lava moat surrounding the pedestal base
//   - Radial lava channels flowing outward from the moat (report 6.x:
//     "Lava Channels: An emissive orange/red texture, with a glow effect.
//      Will 'flow' throughout the volcano and outside it")
//   - Jagged rock-formation borders around the tee fairway
//   - Cup + flag dead-centre on the pedestal
// =============================================================================
class Hole14
{
public:
    Hole14();
    ~Hole14();

    void init(GLuint shaderProgram);
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& lightDir,
              const glm::vec3& viewPos) const;
    void cleanup();

    glm::vec3 cupPosition() const { return _cupPos; }
    glm::vec3 teePosition() const { return _teePos; }
    glm::vec3 sceneCenter() const { return _sceneCenter; }
    float     sceneRadius() const { return _sceneRadius; }

private:
    void build();

    GLuint  _vao, _vbo, _ebo;
    GLsizei _indexCount;
    GLuint  _program;

    GLint _uMVP, _uModel, _uNormalMat;
    GLint _uLightDir, _uLightColor, _uAmbient, _uViewPos;

    glm::vec3 _teePos, _cupPos, _sceneCenter;
    float     _sceneRadius;
};

#endif