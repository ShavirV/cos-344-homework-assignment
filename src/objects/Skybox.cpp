#include "Skybox.h"
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// =============================================================================
Skybox::Skybox()  = default;
Skybox::~Skybox()
{
    if (vao)     glDeleteVertexArrays(1, &vao);
    if (vbo)     glDeleteBuffers(1, &vbo);
    if (ebo)     glDeleteBuffers(1, &ebo);
    if (discVAO) glDeleteVertexArrays(1, &discVAO);
    if (discVBO) glDeleteBuffers(1, &discVBO);
    if (shader)  glDeleteProgram(shader);
    if (texDay)   glDeleteTextures(1, &texDay);
    if (texNight) glDeleteTextures(1, &texNight);
}

// =============================================================================
bool Skybox::init(const char* vertPath, const char* fragPath,
                  const char* dayTexturePath, const char* nightTexturePath)
{
    if (!buildShader(vertPath, fragPath)) return false;

    if (dayTexturePath) {
        hasTexDay = loadBMP(dayTexturePath, texDay);
        if (!hasTexDay)
            std::cerr << "[Skybox] Day texture failed — using gradient\n";
    }
    if (nightTexturePath) {
        hasTexNight = loadBMP(nightTexturePath, texNight);
        if (!hasTexNight)
            std::cerr << "[Skybox] Night texture failed — using gradient\n";
    }

    buildSphere();
    buildDiscs();
    return true;
}

// =============================================================================
void Skybox::update(float dt)
{
    float fullCircle = 2.0f * (float)M_PI;
    angleRad += fullCircle * (dt / dayLengthSecs);
    if (angleRad > fullCircle) angleRad -= fullCircle;
}

// =============================================================================
glm::mat4 Skybox::rotationMatrix() const
{
    // Rotate around world X axis — sun rises from +Z (south), sets toward -Z.
    return glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(1.0f, 0.0f, 0.0f));
}

// =============================================================================
SkyLight Skybox::getSun() const
{
    glm::mat4 rot   = rotationMatrix();
    glm::vec4 local = glm::vec4(localSunDir() * radius, 1.0f);
    glm::vec3 pos   = glm::vec3(rot * local);

    // Intensity: 0 when below horizon (pos.y < 0), 1 at zenith
    float t = glm::clamp(pos.y / radius, 0.0f, 1.0f);

    // Colour shifts warmer near horizon (sunrise/sunset)
    float horizBlend = 1.0f - t;
    glm::vec3 col = glm::mix(sunColor,
                             glm::vec3(1.0f, 0.55f, 0.15f),  // orange at horizon
                             horizBlend * horizBlend);

    return { pos, col, t };
}

SkyLight Skybox::getMoon() const
{
    glm::mat4 rot   = rotationMatrix();
    glm::vec4 local = glm::vec4(localMoonDir() * radius, 1.0f);
    glm::vec3 pos   = glm::vec3(rot * local);

    float t = glm::clamp(pos.y / radius, 0.0f, 1.0f);
    return { pos, moonColor, t * 0.25f };   // moon is much dimmer
}

// =============================================================================
void Skybox::draw(const glm::mat4& view, const glm::mat4& projection)
{
    // Remove translation from view so the skybox stays centred on the camera.
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));

    glUseProgram(shader);

    glUniformMatrix4fv(loc("view"),       1, GL_FALSE, glm::value_ptr(viewNoTrans));
    glUniformMatrix4fv(loc("projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(loc("rotation"),   1, GL_FALSE, glm::value_ptr(rotationMatrix()));

    // Sky gradient colours
    float dayBlend = glm::clamp(getSun().intensity, 0.0f, 1.0f);
    glm::vec3 topCol   = glm::mix(skyNightTop,   skyDayTop,   dayBlend);
    glm::vec3 horizCol = glm::mix(skyNightHoriz, skyDayHoriz, dayBlend);
    glUniform3fv(loc("skyTopColor"),   1, glm::value_ptr(topCol));
    glUniform3fv(loc("skyHorizColor"), 1, glm::value_ptr(horizCol));
    glUniform1f (loc("radius"),        radius);

    // Sun/moon properties for disc rendering
    SkyLight sun  = getSun();
    SkyLight moon = getMoon();
    glUniform3fv(loc("sunLocalDir"),  1, glm::value_ptr(localSunDir()));
    glUniform3fv(loc("moonLocalDir"), 1, glm::value_ptr(localMoonDir()));
    glUniform3fv(loc("sunColor"),     1, glm::value_ptr(sun.color));
    glUniform3fv(loc("moonColor"),    1, glm::value_ptr(moon.color));
    glUniform1f (loc("sunDiscCos"),   cosf(glm::radians(sunDiscSize  * 0.5f)));
    glUniform1f (loc("moonDiscCos"),  cosf(glm::radians(moonDiscSize * 0.5f)));
    // dayBlend is already computed above (0=night, 1=day)
    glUniform1f(loc("texBlend"),    dayBlend);
    glUniform1i(loc("hasTexDay"),   hasTexDay   ? 1 : 0);
    glUniform1i(loc("hasTexNight"), hasTexNight ? 1 : 0);

    if (hasTexDay) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texDay);
        glUniform1i(loc("skyTexDay"), 0);
    }
    if (hasTexNight) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texNight);
        glUniform1i(loc("skyTexNight"), 1);
    }

    // Skybox render state:
    //   - Depth test ON but write OFF (sky is always behind everything)
    //   - Cull front faces (we're inside the sphere)
    //   - No blending needed
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);  // safer: no culling (we're inside sphere)

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    glUseProgram(0);
}

// =============================================================================
// buildSphere
// Generates a UV sphere. Vertices store: vec3 pos, vec2 uv, vec3 normal
// =============================================================================
void Skybox::buildSphere()
{
    int stacks = sphereStacks;
    int slices = sphereSlices;

    int vertCount = (stacks + 1) * (slices + 1);
    int idxCount  = stacks * slices * 6;

    // 8 floats per vertex: pos(3) + uv(2) + normal(3)
    float*        verts   = (float*)        malloc((size_t)(vertCount * 8) * sizeof(float));
    unsigned int* indices = (unsigned int*) malloc((size_t)idxCount       * sizeof(unsigned int));

    if (!verts || !indices) {
        std::cerr << "[Skybox] malloc failed\n";
        free(verts); free(indices); return;
    }

    int vi = 0;
    for (int i = 0; i <= stacks; i++) {
        float phi = (float)M_PI * ((float)i / stacks);  // 0 (top) to PI (bottom)
        float y   = cosf(phi);
        float r   = sinf(phi);
        float v   = (float)i / stacks;

        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * (float)M_PI * ((float)j / slices);
            float x = r * cosf(theta);
            float z = r * sinf(theta);
            float u = (float)j / slices;

            // Position (scaled to radius in shader via rotation matrix * radius)
            verts[vi++] = x;
            verts[vi++] = y;
            verts[vi++] = z;
            // UV
            verts[vi++] = u;
            verts[vi++] = v;
            // Normal (inward — we're inside the sphere)
            verts[vi++] = -x;
            verts[vi++] = -y;
            verts[vi++] = -z;
        }
    }

    int ii = 0;
    int cols = slices + 1;
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            unsigned int tl = (unsigned int)( i      * cols + j     );
            unsigned int tr = (unsigned int)( i      * cols + j + 1 );
            unsigned int bl = (unsigned int)((i + 1) * cols + j     );
            unsigned int br = (unsigned int)((i + 1) * cols + j + 1 );
            // Winding reversed (CCW from inside)
            indices[ii++] = tl; indices[ii++] = tr; indices[ii++] = bl;
            indices[ii++] = tr; indices[ii++] = br; indices[ii++] = bl;
        }
    }
    indexCount = ii;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertCount * 8 * sizeof(float)),
                 verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(idxCount * sizeof(unsigned int)),
                 indices, GL_STATIC_DRAW);

    // location 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*)0);
    // location 1: uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*)(3 * sizeof(float)));
    // location 2: normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*)(5 * sizeof(float)));

    glBindVertexArray(0);
    free(verts);
    free(indices);
}

// =============================================================================
// buildDiscs — not used as separate geometry; sun/moon are rendered in the
// fragment shader as disc halos around the pole directions.
// This function is a no-op stub kept for future billboard geometry.
// =============================================================================
void Skybox::buildDiscs() { /* handled in fragment shader */ }

// =============================================================================
// loadBMP — identical to Water's BMP loader (stdio only)
// =============================================================================
bool Skybox::loadBMP(const char* path, GLuint& outTex)
{
    FILE* f = fopen(path, "rb");
    if (!f) { std::cerr << "[Skybox] Cannot open BMP: " << path << "\n"; return false; }

    unsigned char fh[14];
    if (fread(fh, 1, 14, f) != 14 || fh[0]!='B' || fh[1]!='M') {
        std::cerr << "[Skybox] Not a BMP: " << path << "\n";
        fclose(f); return false;
    }

    unsigned int dataOffset = (unsigned int)fh[10] | ((unsigned int)fh[11]<<8)
                            | ((unsigned int)fh[12]<<16)| ((unsigned int)fh[13]<<24);

    unsigned char dib[40];
    fread(dib, 1, 40, f);

    int w = (int)((unsigned int)dib[4]|((unsigned int)dib[5]<<8)
                 |((unsigned int)dib[6]<<16)|((unsigned int)dib[7]<<24));
    int h = (int)((unsigned int)dib[8]|((unsigned int)dib[9]<<8)
                 |((unsigned int)dib[10]<<16)|((unsigned int)dib[11]<<24));
    unsigned short bpp  = (unsigned short)((unsigned int)dib[14]|((unsigned int)dib[15]<<8));
    unsigned int   comp = (unsigned int)dib[16]|((unsigned int)dib[17]<<8)
                         |((unsigned int)dib[18]<<16)|((unsigned int)dib[19]<<24);

    if (bpp != 24 || comp != 0) {
        std::cerr << "[Skybox] BMP must be 24-bit uncompressed\n";
        fclose(f); return false;
    }

    bool flip = (h > 0);
    if (h < 0) h = -h;
    int padded = (w * 3 + 3) & ~3;

    unsigned char* raw = (unsigned char*)malloc((size_t)(padded * h));
    if (!raw) { fclose(f); return false; }
    fseek(f, (long)dataOffset, SEEK_SET);
    fread(raw, 1, (size_t)(padded * h), f);
    fclose(f);

    unsigned char* px = (unsigned char*)malloc((size_t)(w * h * 3));
    if (!px) { free(raw); return false; }

    for (int row = 0; row < h; row++) {
        int src = flip ? (h - 1 - row) : row;
        unsigned char* s = raw + src * padded;
        unsigned char* d = px  + row * w * 3;
        for (int col = 0; col < w; col++) {
            d[col*3+0] = s[col*3+2];
            d[col*3+1] = s[col*3+1];
            d[col*3+2] = s[col*3+0];
        }
    }
    free(raw);

    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_2D, outTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    free(px);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

// =============================================================================
// buildShader — same pattern as Water, using stdio only
// =============================================================================
static GLuint skyCompileStage(GLenum type, const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) { std::cerr << "[Skybox] Cannot open: " << path << "\n"; return 0; }
    fseek(f, 0, SEEK_END); long len = ftell(f); rewind(f);
    char* src = (char*)malloc((size_t)(len + 1));
    if (!src) { fclose(f); return 0; }
    fread(src, 1, (size_t)len, f); src[len] = '\0'; fclose(f);

    GLuint id = glCreateShader(type);
    const char* cs = src;
    glShaderSource(id, 1, &cs, nullptr);
    glCompileShader(id);
    free(src);

    GLint ok = 0; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(id, 1024, nullptr, log);
        std::cerr << "[Skybox] Compile error (" << path << "):\n" << log << "\n";
        glDeleteShader(id); return 0;
    }
    return id;
}

bool Skybox::buildShader(const char* vertPath, const char* fragPath)
{
    GLuint v = skyCompileStage(GL_VERTEX_SHADER,   vertPath);
    GLuint f = skyCompileStage(GL_FRAGMENT_SHADER, fragPath);
    if (!v || !f) { glDeleteShader(v); glDeleteShader(f); return false; }

    shader = glCreateProgram();
    glAttachShader(shader, v);
    glAttachShader(shader, f);
    glLinkProgram(shader);
    GLint ok = 0; glGetProgramiv(shader, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(shader, 1024, nullptr, log);
        std::cerr << "[Skybox] Link error:\n" << log << "\n";
        glDeleteProgram(shader); shader = 0;
    }
    glDeleteShader(v); glDeleteShader(f);
    return ok != 0;
}

GLint Skybox::loc(const char* name) const {
    return glGetUniformLocation(shader, name);
}