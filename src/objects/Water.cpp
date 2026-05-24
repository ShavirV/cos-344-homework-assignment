#include "Water.h"

// Allowed: stdio.h, stdlib.h, iostream, cmath, GL/glew.h, glm
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>

// -------------------------------------------------------
// Constructor / Destructor
// -------------------------------------------------------
WaterMesh::WaterMesh()  = default;

WaterMesh::~WaterMesh() {
    if (vao)     glDeleteVertexArrays(1, &vao);
    if (vbo)     glDeleteBuffers(1, &vbo);
    if (ebo)     glDeleteBuffers(1, &ebo);
    if (shader)  glDeleteProgram(shader);
    if (tex0)    glDeleteTextures(1, &tex0);
    if (tex1)    glDeleteTextures(1, &tex1);
    if (texFoam) glDeleteTextures(1, &texFoam);
}

// -------------------------------------------------------
// init
// -------------------------------------------------------
bool WaterMesh::init(const char* vertPath,
                     const char* fragPath,
                     const char* normalMap0Path,
                     const char* normalMap1Path,
                     const char* foamMapPath)
{
    if (!buildShader(vertPath, fragPath)) return false;
    if (!loadBMP(normalMap0Path, tex0))   return false;
    if (!loadBMP(normalMap1Path, tex1))   return false;
    if (!loadBMP(foamMapPath,    texFoam))return false;
    buildMesh();
    return true;
}

// -------------------------------------------------------
// update
// -------------------------------------------------------
void WaterMesh::update(float deltaTime) {
    accumTime += deltaTime;
}

// -------------------------------------------------------
// draw
// -------------------------------------------------------
void WaterMesh::draw(const glm::mat4& view,
                     const glm::mat4& projection,
                     const glm::vec3& cameraPos,
                     const glm::vec3& lightDir,
                     const glm::vec3& lightColor)
{
    glUseProgram(shader);

    glUniformMatrix4fv(loc("model"),      1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(loc("view"),       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(loc("projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(loc("viewPos"),          1, glm::value_ptr(cameraPos));
    glUniform1f(loc("time"), accumTime);

    glm::vec3 ld = glm::normalize(lightDir);
    glUniform3fv(loc("lightDir"),   1, glm::value_ptr(ld));
    glUniform3fv(loc("lightColor"), 1, glm::value_ptr(lightColor));

    glUniform3fv(loc("waterColorShallow"), 1, glm::value_ptr(colorShallow));
    glUniform3fv(loc("waterColorDeep"),    1, glm::value_ptr(colorDeep));

    glUniform1f(loc("normalTile0"),   normalTile0);
    glUniform1f(loc("normalTile1"),   normalTile1);
    glUniform1f(loc("scrollSpeed0"),  scrollSpeed0);
    glUniform1f(loc("scrollSpeed1"),  scrollSpeed1);
    glUniform1f(loc("foamTile"),      foamTile);
    glUniform1f(loc("foamThreshold"), foamThreshold);
    glUniform1f(loc("fresnelBias"),   fresnelBias);
    glUniform1f(loc("fresnelScale"),  fresnelScale);
    glUniform1f(loc("fresnelPower"),  fresnelPower);
    glUniform1f(loc("specularStrength"), specularStrength);
    glUniform1f(loc("shininess"),        shininess);

    for (int i = 0; i < 4; i++) {
        // Build the uniform name manually — no std::string allowed.
        // "waves[0]" .. "waves[3]"
        char name[16];
        name[0]='w'; name[1]='a'; name[2]='v'; name[3]='e'; name[4]='s';
        name[5]='['; name[6]=(char)('0'+i); name[7]=']'; name[8]='\0';
        glm::vec4 w(waves[i].direction.x,
                    waves[i].direction.y,
                    waves[i].steepness,
                    waves[i].wavelength);
        glUniform4fv(glGetUniformLocation(shader, name), 1, glm::value_ptr(w));
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    glUniform1i(loc("normalMap0"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex1);
    glUniform1i(loc("normalMap1"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texFoam);
    glUniform1i(loc("foamMap"), 2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUseProgram(0);
}

// -------------------------------------------------------
// buildMesh
// -------------------------------------------------------
void WaterMesh::buildMesh()
{
    int   N  = gridResolution;
    float hw = planeWidth  * 0.5f;
    float hl = planeLength * 0.5f;

    // Interleaved: vec3 pos, vec2 uv  (20 bytes per vertex)
    int vertCount = (N + 1) * (N + 1);
    int idxCount  = N * N * 6;

    float*        verts   = (float*)        malloc(vertCount * 5 * sizeof(float));
    unsigned int* indices = (unsigned int*) malloc(idxCount  *     sizeof(unsigned int));

    if (!verts || !indices) {
        std::cerr << "[Water] malloc failed in buildMesh\n";
        free(verts); free(indices);
        return;
    }

    int vi = 0;
    for (int z = 0; z <= N; z++) {
        for (int x = 0; x <= N; x++) {
            float fx = (float)x / N;
            float fz = (float)z / N;
            verts[vi++] = -hw + fx * planeWidth;   // pos X
            verts[vi++] = 0.0f;                     // pos Y
            verts[vi++] = -hl + fz * planeLength;  // pos Z
            verts[vi++] = fx;                       // uv U
            verts[vi++] = fz;                       // uv V
        }
    }

    int ii = 0;
    for (int z = 0; z < N; z++) {
        for (int x = 0; x < N; x++) {
            unsigned int tl = (unsigned int)( z      * (N+1) + x   );
            unsigned int tr = (unsigned int)( z      * (N+1) + x+1 );
            unsigned int bl = (unsigned int)((z+1)   * (N+1) + x   );
            unsigned int br = (unsigned int)((z+1)   * (N+1) + x+1 );
            indices[ii++] = tl; indices[ii++] = bl; indices[ii++] = tr;
            indices[ii++] = tr; indices[ii++] = bl; indices[ii++] = br;
        }
    }
    indexCount = ii;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertCount * 5 * sizeof(float)),
                 verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(idxCount * sizeof(unsigned int)),
                 indices, GL_STATIC_DRAW);

    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)0);

    // location 1: texcoord (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    free(verts);
    free(indices);
}

// -------------------------------------------------------
// loadBMP
// Reads a 24-bit uncompressed Windows BMP using only
// stdio.h and stdlib.h (both on the allowed list).
// Converts BGR → RGB and uploads to GL_TEXTURE_2D.
// -------------------------------------------------------
bool WaterMesh::loadBMP(const char* path, GLuint& outTex)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        std::cerr << "[Water] Cannot open BMP: " << path << "\n";
        return false;
    }

    // ── BMP file header (14 bytes) ──────────────────────
    unsigned char fileHeader[14];
    if (fread(fileHeader, 1, 14, f) != 14) {
        std::cerr << "[Water] Bad BMP file header: " << path << "\n";
        fclose(f); return false;
    }
    if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
        std::cerr << "[Water] Not a BMP file: " << path << "\n";
        fclose(f); return false;
    }

    // Pixel data offset (bytes 10-13, little-endian)
    unsigned int dataOffset =
        (unsigned int)fileHeader[10]        |
        ((unsigned int)fileHeader[11] << 8) |
        ((unsigned int)fileHeader[12] << 16)|
        ((unsigned int)fileHeader[13] << 24);

    // ── DIB header — read width, height, bitcount ────────
    unsigned char dibHeader[40];
    if (fread(dibHeader, 1, 40, f) != 40) {
        std::cerr << "[Water] Bad DIB header: " << path << "\n";
        fclose(f); return false;
    }

    int width  = (int)( (unsigned int)dibHeader[4]  |
                       ((unsigned int)dibHeader[5]  << 8) |
                       ((unsigned int)dibHeader[6]  << 16)|
                       ((unsigned int)dibHeader[7]  << 24));
    int height = (int)( (unsigned int)dibHeader[8]  |
                       ((unsigned int)dibHeader[9]  << 8) |
                       ((unsigned int)dibHeader[10] << 16)|
                       ((unsigned int)dibHeader[11] << 24));
    unsigned short bpp = (unsigned short)(
                       (unsigned int)dibHeader[14] |
                       ((unsigned int)dibHeader[15] << 8));
    unsigned int compression = (unsigned int)dibHeader[16] |
                              ((unsigned int)dibHeader[17] << 8) |
                              ((unsigned int)dibHeader[18] << 16)|
                              ((unsigned int)dibHeader[19] << 24);

    if (bpp != 24 || compression != 0) {
        std::cerr << "[Water] BMP must be 24-bit uncompressed: " << path
                  << " (bpp=" << bpp << " comp=" << compression << ")\n";
        fclose(f); return false;
    }

    // Handle bottom-up (positive height) and top-down (negative height) BMPs
    bool flipRows = (height > 0);
    if (height < 0) height = -height;

    // BMP rows are padded to a multiple of 4 bytes
    int rowBytes   = width * 3;
    int paddedRow  = (rowBytes + 3) & ~3;
    int dataSize   = paddedRow * height;

    unsigned char* raw = (unsigned char*)malloc((size_t)dataSize);
    if (!raw) {
        std::cerr << "[Water] malloc failed for BMP data\n";
        fclose(f); return false;
    }

    fseek(f, (long)dataOffset, SEEK_SET);
    if (fread(raw, 1, (size_t)dataSize, f) != (size_t)dataSize) {
        std::cerr << "[Water] Short read on BMP pixel data: " << path << "\n";
        free(raw); fclose(f); return false;
    }
    fclose(f);

    // Convert BGR → RGB, strip row padding, optionally flip rows
    unsigned char* pixels = (unsigned char*)malloc((size_t)(width * height * 3));
    if (!pixels) { free(raw); return false; }

    for (int row = 0; row < height; row++) {
        int srcRow = flipRows ? (height - 1 - row) : row;
        unsigned char* src = raw + srcRow * paddedRow;
        unsigned char* dst = pixels + row * width * 3;
        for (int col = 0; col < width; col++) {
            dst[col*3 + 0] = src[col*3 + 2]; // R ← B
            dst[col*3 + 1] = src[col*3 + 1]; // G ← G
            dst[col*3 + 2] = src[col*3 + 0]; // B ← R
        }
    }
    free(raw);

    // Upload to OpenGL
    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_2D, outTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

// -------------------------------------------------------
// buildShader — reads GLSL source files via stdio, compiles
// using the same pattern as the existing shader.hpp.
// -------------------------------------------------------
static GLuint compileStage(GLenum type, const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) {
        std::cerr << "[Water] Cannot open shader: " << path << "\n";
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char* src = (char*)malloc((size_t)(len + 1));
    if (!src) { fclose(f); return 0; }
    fread(src, 1, (size_t)len, f);
    src[len] = '\0';
    fclose(f);

    GLuint id = glCreateShader(type);
    const char* cs = src;
    glShaderSource(id, 1, &cs, nullptr);
    glCompileShader(id);
    free(src);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, 1024, nullptr, log);
        std::cerr << "[Water] Shader compile error (" << path << "):\n" << log << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool WaterMesh::buildShader(const char* vertPath, const char* fragPath)
{
    GLuint vert = compileStage(GL_VERTEX_SHADER,   vertPath);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragPath);
    if (!vert || !frag) { glDeleteShader(vert); glDeleteShader(frag); return false; }

    shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);

    GLint ok = 0;
    glGetProgramiv(shader, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(shader, 1024, nullptr, log);
        std::cerr << "[Water] Shader link error:\n" << log << "\n";
        glDeleteProgram(shader);
        shader = 0;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return ok != 0;
}

// -------------------------------------------------------
// loc — uniform location helper
// -------------------------------------------------------
GLint WaterMesh::loc(const char* name) const {
    return glGetUniformLocation(shader, name);
}