// Trex.hpp - drop-in OpenGL 3.3 wrapper for the T-rex mesh.
// COS344 Practical: only uses allowed libraries (GLEW, glm).
//
// Usage:
//   #include "Trex.hpp"
//   Trex trex;
//   trex.init();                                      // after glewInit()
//   trex.draw(modelLoc, glm::vec3(40, 0, -10), 0.4f); // each frame
//   trex.cleanup();                                   // on shutdown
//
// Default size: ~12.5 m long, ~5.6 m tall (Blender +Z up). Pass a scale
// like 0.4 for a roughly 5 m mini-golf decoration, or 1.0 for a full
// landmark.

#ifndef TREX_HPP
#define TREX_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../assets/models/trex_mesh.h"

class Trex {
public:
    Trex() : vao(0), vbo(0), ebo(0), initialized(false) {}

    void init() {
        if (initialized) return;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TREX_VERTICES), TREX_VERTICES, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TREX_INDICES), TREX_INDICES, GL_STATIC_DRAW);

        // Vertex layout: 11 floats = pos(3) | normal(3) | uv(2) | color(3)
        const GLsizei stride = 11 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

        glBindVertexArray(0);
        initialized = true;
    }

    // Build a model matrix that places the T-rex.
    //   -90deg rotation about X: convert Blender +Z up -> OpenGL +Y up.
    //   Right-hand rule: rotating -90° about +X sends Blender's up vector
    //   (0,0,1) to (0,1,0). The opposite sign (+90°) sends it to (0,-1,0),
    //   which is upside down.
    //   yawRad: rotate around world +Y so the T-rex faces a chosen direction.
    //   placement: world position of the T-rex's feet center.
    //
    // Order (read bottom-to-top, that's the application order):
    //   1. axis rotation (Z-up to Y-up) — done first on the raw mesh
    //   2. scale          — operates in the already-uprighted frame
    //   3. world yaw      — spins about world Y
    //   4. translate      — final world placement
    glm::mat4 modelMatrix(const glm::vec3& placement = glm::vec3(0.0f),
                          float scale = 1.0f,
                          float yawRad = 0.0f) const {
        glm::mat4 m(1.0f);
        m = glm::translate(m, placement);
        m = glm::rotate(m, yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::scale(m, glm::vec3(scale));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        return m;
    }

    void draw(GLint modelLoc = -1,
              const glm::vec3& placement = glm::vec3(0.0f),
              float scale = 1.0f,
              float yawRad = 0.0f) const {
        if (modelLoc >= 0) {
            glm::mat4 m = modelMatrix(placement, scale, yawRad);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        }
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, TREX_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void cleanup() {
        if (!initialized) return;
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        initialized = false;
    }

    ~Trex() { cleanup(); }

    static float bodyLength() { return 12.5f; }
    static float standingHeight() { return 5.6f; }

private:
    GLuint vao, vbo, ebo;
    bool   initialized;
};

#endif // TREX_HPP