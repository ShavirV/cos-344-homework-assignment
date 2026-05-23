#ifndef HOLE15_H
#define HOLE15_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =============================================================================
// Hole 15 - "Volcano Approach Ramp"
//
// Report (4.1): "The fairway rises along the base of the central volcano
// structure and curves around it. Rendered as a curved ramp section using
// a series of rotated and connected quad strips."
//
// This class builds JUST the ramp + surrounding gameplay geometry.
// The volcano itself is intentionally NOT rendered here (a Blender asset
// will be slotted into the inner-radius hollow at integration time).
//
// Implementation:
//   - Wide curved fairway sweeping ~170 degrees around the empty centre
//   - Smooth-shaded quad strips with finite-difference normals
//   - Tall basalt parapet along the outer rim of the ramp
//   - Inner kerb wall on the volcano side
//   - Outer skirt (basalt) closing the ramp underside down to ground
//   - Boulders scattered along the ramp surface as obstacles
//   - Tee pad at the start (low end), cup + flag at the top (high end)
// =============================================================================
class Hole15
{
public:
    Hole15();
    ~Hole15();

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