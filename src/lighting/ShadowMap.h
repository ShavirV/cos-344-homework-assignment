#pragma once
// =============================================================================
// ShadowMap.h
//
// Owns the resources used by the shadow map depth pre-pass:
//   - a depth-only framebuffer (no colour attachment)
//   - a 2D depth texture (the "shadow map")
//   - the shadow.vert / shadow.frag program
//
// Usage per frame:
//   sm.beginPass(sunDir);                  // bind FBO, clear depth, glViewport
//   for each renderable r:                 //   ← every hole / volcano / ground
//       r.drawDepth(sm.depthShader());
//   sm.endPass(screenWidth, screenHeight); // restore default FBO + viewport
//
//   // Then in your colour pass, bind sm.depthTexture() to a sampler unit
//   // and upload sm.lightSpaceMatrix() to the main shader. (That part comes
//   // in Step 2 — for now, the depth pass just runs and populates the map.)
//
// Spec-allowed libs only (GLEW + glm).
// =============================================================================

#include <GL/glew.h>
#include <glm/glm.hpp>

class ShadowMap
{
public:
    ShadowMap();
    ~ShadowMap();

    // Build the FBO, depth texture (size × size), and load the shadow shader
    // pair. Returns false on any failure (logged to stderr).
    bool init(int   size = 2048,
              const char* vertPath = "src/shaders/shadow.vert",
              const char* fragPath = "src/shaders/shadow.frag");

    // Configure the orthographic frustum used to project from the sun's POV.
    // Defaults cover roughly the 400×150 ground plane comfortably.
    void setSceneBounds(const glm::vec3& centre, float radius);

    // Compute and cache the light-space matrix = lightProj * lightView.
    //   sunDir : unit vector pointing FROM the surface TOWARD the sun
    //            (same convention used by fragment.glsl's uLightDir).
    // Called automatically by beginPass; exposed here so the colour pass
    // can reuse the same matrix without recomputing.
    const glm::mat4& lightSpaceMatrix() const { return mLightSpace; }
    glm::mat4 computeLightSpaceMatrix(const glm::vec3& sunDir);

    // Bind the FBO + shadow shader, set the viewport to the shadow map size,
    // clear depth, and upload uLightSpaceMatrix. After this call, every
    // drawDepth(shadowShader) issued will populate the depth texture.
    void beginPass(const glm::vec3& sunDir);

    // Unbind FBO, restore the default framebuffer + viewport.
    void endPass(int screenW, int screenH);

    // Accessors for the colour pass.
    GLuint depthTexture() const { return mDepthTex; }
    GLuint depthShader()  const { return mShader;   }
    int    size()         const { return mSize;     }

private:
    GLuint mFBO        = 0;
    GLuint mDepthTex   = 0;
    GLuint mShader     = 0;
    int    mSize       = 0;
    bool   mInitialised= false;

    // Orthographic light-frustum extents (set by setSceneBounds; defaults set
    // in init()). The frustum is a box of side 2*mRadius centred on
    // mSceneCentre, with the light placed mRadius units back along sunDir.
    glm::vec3 mSceneCentre = glm::vec3(0.0f);
    float     mRadius      = 250.0f;

    glm::mat4 mLightSpace  = glm::mat4(1.0f);
};