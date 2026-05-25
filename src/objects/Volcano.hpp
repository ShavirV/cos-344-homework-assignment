// Volcano.hpp - drop-in OpenGL 3.3 wrapper for the volcano mesh.
// COS344 Practical: uses only allowed libraries (GLEW, GLFW, glm, cmath, etc.)
//
// Usage:
//   #include "Volcano.hpp"
//   Volcano vol;            // create AFTER your GL context exists
//   vol.init();             // uploads VBO/EBO once
//   vol.draw(modelLoc);     // call inside your render loop; pass the uniform location of "model"
//   vol.cleanup();          // on shutdown
//
// The mesh is +Z up (Blender convention). The default model matrix returned by
// modelMatrix() rotates it into +Y up to match a typical OpenGL world. Override
// it freely (translate / scale / rotate) when placing the volcano on your course.

#ifndef VOLCANO_HPP
#define VOLCANO_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../assets/models/volcano_mesh.h" // generated data: VOLCANO_VERTICES, VOLCANO_INDICES, counts

class Volcano {
public:
    Volcano() : vao(0), vbo(0), ebo(0), initialized(false) {}

    // Build VAO/VBO/EBO. Must be called after glewInit() and a current GL context.
    void init() {
        if (initialized) return;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(VOLCANO_VERTICES),
                     VOLCANO_VERTICES,
                     GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(VOLCANO_INDICES),
                     VOLCANO_INDICES,
                     GL_STATIC_DRAW);

        // Vertex layout (11 floats per vertex):
        //   location 0: vec3 aPos
        //   location 1: vec3 aNormal
        //   location 2: vec2 aUV
        //   location 3: vec3 aColor   (rock/lava vertex color)
        const GLsizei stride = 11 * sizeof(float);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                              (void*)(0 * sizeof(float)));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                              (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                              (void*)(6 * sizeof(float)));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                              (void*)(8 * sizeof(float)));

        glBindVertexArray(0);
        initialized = true;
    }

    // Returns the default model matrix:
    //   1) rotate so +Z up (Blender) becomes +Y up (OpenGL world).
    //   2) translate by `placement`.
    //   3) uniform scale by `scale`.
    glm::mat4 modelMatrix(const glm::vec3& placement = glm::vec3(0.0f),
                          float scale = 1.0f) const {
        glm::mat4 m(1.0f);
        m = glm::translate(m, placement);
        m = glm::scale(m, glm::vec3(scale));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        return m;
    }

    // Draw with a pre-set shader. modelLoc is the uniform location of "model"
    // (mat4). If you handle your model matrix elsewhere, pass -1 and ignore.
    void draw(GLint modelLoc = -1,
              const glm::vec3& placement = glm::vec3(0.0f),
              float scale = 1.0f) const {
        if (modelLoc >= 0) {
            glm::mat4 m = modelMatrix(placement, scale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        }
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, VOLCANO_INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void cleanup() {
        if (!initialized) return;
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        initialized = false;
    }

    ~Volcano() { cleanup(); }

    // Bounding info (handy for camera/collisions)
    static float baseRadius() { return 6.0f; }
    static float height()     { return 12.0f; }

private:
    GLuint vao, vbo, ebo;
    bool   initialized;
};

#endif // VOLCANO_HPP
