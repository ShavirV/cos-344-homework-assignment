#include "ShadowMap.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// =============================================================================
// Local helper: load + compile + link a vertex/fragment GLSL pair.
// Same stdio-based loader pattern as Skybox.cpp.
// =============================================================================
static GLuint compileStage(GLenum type, const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) { std::cerr << "[ShadowMap] Cannot open: " << path << "\n"; return 0; }
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
        std::cerr << "[ShadowMap] Compile error (" << path << "):\n" << log << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static GLuint linkProgram(GLuint v, GLuint f)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << "[ShadowMap] Link error:\n" << log << "\n";
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

// =============================================================================
ShadowMap::ShadowMap()  = default;
ShadowMap::~ShadowMap()
{
    if (mDepthTex) glDeleteTextures(1, &mDepthTex);
    if (mFBO)      glDeleteFramebuffers(1, &mFBO);
    if (mShader)   glDeleteProgram(mShader);
}

// =============================================================================
bool ShadowMap::init(int size, const char* vertPath, const char* fragPath)
{
    mSize = size;

    // ── Depth texture ────────────────────────────────────────────────────────
    // Single-channel depth attachment. GL_DEPTH_COMPONENT24 is the safest
    // pick on a 3.3 core context — 32-bit float depth works on most GPUs
    // but isn't guaranteed without ARB_depth_buffer_float, and 16-bit shows
    // banding under steep light angles.
    glGenTextures(1, &mDepthTex);
    glBindTexture(GL_TEXTURE_2D, mDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 mSize, mSize, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // Sample with bilinear filtering for soft shadow edges (PCF will use
    // textureGather in the colour pass; bilinear here keeps stray sampling
    // smooth).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Clamp to border + a border colour of 1.0 — fragments OUTSIDE the
    // shadow frustum should be treated as fully lit (depth = 1.0 = far).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ── Framebuffer with depth-only attachment ───────────────────────────────
    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, mDepthTex, 0);
    // Critical: tell GL we have no colour attachment, otherwise the
    // framebuffer is incomplete on most drivers.
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ShadowMap] FBO incomplete: 0x" << std::hex << status << "\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── Shader pair ──────────────────────────────────────────────────────────
    GLuint v = compileStage(GL_VERTEX_SHADER,   vertPath);
    GLuint f = compileStage(GL_FRAGMENT_SHADER, fragPath);
    if (!v || !f) { glDeleteShader(v); glDeleteShader(f); return false; }
    mShader = linkProgram(v, f);
    glDeleteShader(v); glDeleteShader(f);
    if (!mShader) return false;

    mInitialised = true;
    std::cerr << "[ShadowMap] init OK — " << mSize << "x" << mSize << " depth map\n";
    return true;
}

// =============================================================================
void ShadowMap::setSceneBounds(const glm::vec3& centre, float radius)
{
    mSceneCentre = centre;
    mRadius      = radius;
}

// =============================================================================
// computeLightSpaceMatrix
//
// Build the matrix that transforms world-space positions into the sun's clip
// space. = projection (orthographic) * view (look from sun toward scene).
//
// The "sun" sits a distance R behind the scene centre along sunDir, looking
// at the centre. The orthographic frustum is a box of side 2R centred on
// that point, deep enough (2.5R near→far) to span the scene end-to-end.
//
// sunDir convention here matches fragment.glsl's uLightDir: it points FROM
// the surface TOWARD the sun, so the sun's position is centre + R * sunDir.
// =============================================================================
glm::mat4 ShadowMap::computeLightSpaceMatrix(const glm::vec3& sunDir)
{
    glm::vec3 nrmSun = glm::normalize(sunDir);
    glm::vec3 lightPos = mSceneCentre + nrmSun * mRadius;

    // Look-at: lightPos → sceneCentre. Up vector must NOT be parallel to
    // the look direction or glm::lookAt produces a singular matrix. The
    // sunDir.y >= 0.10 horizon clamp in main.cpp guarantees the sun is
    // always above horizon, so world-up (0,1,0) is safe except for the
    // degenerate case of sun straight overhead — guard against it.
    glm::vec3 up = (fabsf(nrmSun.y) > 0.99f)
                   ? glm::vec3(0.0f, 0.0f, 1.0f)
                   : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 lightView = glm::lookAt(lightPos, mSceneCentre, up);

    // Orthographic projection: extents R on each side of the centre,
    // 0.1 to 2.5*R for the near/far plane. The far plane is generous so
    // tall objects (volcano, flag poles) aren't clipped against it.
    float r = mRadius;
    glm::mat4 lightProj = glm::ortho(-r, r, -r, r, 0.1f, 2.5f * r);

    mLightSpace = lightProj * lightView;
    return mLightSpace;
}

// =============================================================================
void ShadowMap::beginPass(const glm::vec3& sunDir)
{
    if (!mInitialised) return;

    computeLightSpaceMatrix(sunDir);

    // Bind FBO + shadow program, set viewport to the depth texture size,
    // clear the depth buffer (1.0 = far). The colour buffer doesn't exist
    // here, so don't try to clear it.
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glViewport(0, 0, mSize, mSize);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(mShader);
    GLint loc = glGetUniformLocation(mShader, "uLightSpaceMatrix");
    if (loc != -1)
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mLightSpace));

    // Front-face culling during the depth pass reduces shadow acne on flat
    // surfaces — only back faces of casters write depth. Saved+restored
    // around the pass so main rendering is unaffected.
    // (Disabled for now: your main pass also uses culling off because hole
    // geometry has mixed winding. Re-enable later if shadow acne shows up.)
    // glEnable(GL_CULL_FACE); glCullFace(GL_FRONT);
}

// =============================================================================
void ShadowMap::endPass(int screenW, int screenH)
{
    if (!mInitialised) return;

    // Restore default framebuffer + screen viewport. The caller must reset
    // any other GL state it changed (depth func, blend, etc.) on its own.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
    glUseProgram(0);

    // glDisable(GL_CULL_FACE);   // matches the commented-out enable above
}