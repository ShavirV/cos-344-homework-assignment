// Brachio.hpp - drop-in OpenGL 3.3 wrapper for the Brachiosaurus mesh.
// COS344 Practical: only uses allowed libraries (GLEW, glm).
//
// Usage:
//   #include "Brachio.hpp"
//   Brachio brachio;
//   brachio.init();                                            // after glewInit()
//   brachio.draw(modelLoc, glm::vec3(50, 0, 30), 0.35f, 0.0f); // each frame
//   brachio.cleanup();                                         // on shutdown
//
// Default raw size: ~13 m head-to-tail, ~13 m tall (Blender +Z up).
// Pass scale ≈ 0.35 for a roughly 4.5 m mini-golf decoration.

#ifndef BRACHIO_HPP
#define BRACHIO_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../assets/models/brachio_mesh.h"

class Brachio {
public:
    Brachio() : vao(0), vbo(0), ebo(0), initialized(false) {}

    void init() {
        if (initialized) return;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BRACHIO_VERTICES), BRACHIO_VERTICES, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(BRACHIO_INDICES), BRACHIO_INDICES, GL_STATIC_DRAW);

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

    // Build a model matrix that places the brachiosaurus.
    //   -90deg about X: convert Blender +Z up -> OpenGL +Y up.
    //   yawRad: rotate around world +Y so it faces a chosen direction.
    //   placement: world position of the feet center.
    //
    // Order (read bottom-to-top, that's application order):
    //   1. axis rotation (Z-up -> Y-up)
    //   2. scale (in already-uprighted frame)
    //   3. world yaw (spins about world Y)
    //   4. translate (final world placement)
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
        glDrawElements(GL_TRIANGLES, BRACHIO_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void cleanup() {
        if (!initialized) return;
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        initialized = false;
    }

    ~Brachio() { cleanup(); }

    static float bodyLength()     { return 13.0f; }
    static float standingHeight() { return 13.2f; }

private:
    GLuint vao, vbo, ebo;
    bool   initialized;
};

#endif // BRACHIO_HPP
