#pragma once
// =============================================================================
// Hole14.h
// Hole 14 of the Dino Park miniature golf course — the descending S-curve.
//
// Geometry
// --------
// The tee sits on a raised platform (4m above the cup); the fairway snakes
// down an S-curve to the cup at ground level, with an average descent of ~13°
// (steepest local slope ~20°). Concrete kerbs follow both fairway edges, and
// each kerb has several inward protrusions ("pinches") that narrow the
// playable corridor at a few spots — alternating left/right so the path
// zigzags slightly between obstacles.
//
// Implementation
// --------------
// The fairway is built as a swept ribbon: a centreline parameterised by
// s ∈ [0, 1], with a local frame (tangent, side, up) recomputed at each
// sample. Successive samples are stitched together with quads (top + bottom
// + side strips). The kerb edges follow the same sweep, with their inner
// edge displaced inward by Gaussian "pinch" functions of s — this is what
// produces the protrusions.
//
// All geometry is in local space; the hole sits with its tee at (0, 4, 0)
// and its cup at (0, 0, ~11). sceneCenter() / sceneRadius() are hardcoded
// to a sensible framing — matching the Hole 13 approach.
//
// Interface matches dev.cpp's expectations exactly the same as Hole 13:
// init(program), draw(view, proj, sunDir, camPos), cleanup(),
// sceneCenter(), sceneRadius(). Procedural-texture shading is handled by
// the fragment shader in dev.cpp and works automatically as long as the
// vertex colours are recognisable as turf-green or kerb-grey.
// =============================================================================

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Hole14
{
public:
    Hole14();
    ~Hole14();

    void init(GLuint shaderProgram);
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& sunDir,
              const glm::vec3& camPos);
    void cleanup();

    glm::vec3 sceneCenter() const { return mCenter; }
    float     sceneRadius() const { return mRadius; }

private:
    // -------------------------------------------------------------------------
    // Vertex layout — matches dev.cpp's shader (aPos=0, aNormal=1, aColor=2).
    // Identical to Hole13 so the same shader/draw path works.
    // -------------------------------------------------------------------------
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };

    // -------------------------------------------------------------------------
    // Centreline + cross-section computation
    // -------------------------------------------------------------------------

    // Centreline position at parameter s ∈ [0, 1]. The curve:
    //   x = AMP * sin(2π s)        — S-shape in the XZ plane
    //   y = START_HEIGHT * (1 - s) — linear descent to 0
    //   z = Z_LEN * s              — straight forward in Z
    glm::vec3 centreline(float s) const;

    // Approximate tangent at s by central differencing.
    glm::vec3 tangent(float s) const;

    // The pinch profile that narrows the fairway at certain s values.
    // Returns (leftHalfW, rightHalfW) — the per-side fairway half-widths
    // after the protrusions are applied.
    void pinchProfile(float s, float& leftHalfW, float& rightHalfW) const;

    // -------------------------------------------------------------------------
    // Geometry builders. Each appends triangles to mVertices / mIndices.
    // -------------------------------------------------------------------------

    // Sweep the fairway ribbon along the centreline, with per-sample
    // left/right half-widths so the pinches are baked into the fairway shape
    // directly. Slab thickness is `slabThick`.
    void buildFairwayRibbon(int   sSamples,
                            float slabThick,
                            const glm::vec3& colour);

    // Sweep the kerbs along the centreline. The kerb's inner edge sits on
    // the corresponding fairway edge (so pinches translate to bulges in
    // the kerb's inner edge); the kerb's outer edge sits a fixed thickness
    // further out from a CONSTANT reference width — this way the outer
    // perimeter stays smooth while the inner edge protrudes.
    void buildKerbRibbon(int   sSamples,
                         float kerbH, float kerbT,
                         float baseHalfW,
                         const glm::vec3& colour,
                         bool  leftSide);   // true = left kerb, false = right

    // Tee platform: a small slab at the tee end, extending OUT past the kerb
    // so it reads as a tee box sitting at the top of the hill rather than
    // a continuation of the fairway.
    void buildTeePlatform(const glm::vec3& centre,
                          const glm::vec3& tangent,
                          float halfW, float depth,
                          float slabThick,
                          const glm::vec3& colour);

    // Cup + flag at the bottom of the descent.
    void buildCup(const glm::vec3& centre, const glm::vec3& colour);
    void buildFlag(const glm::vec3& base, float poleH, float poleR,
                   const glm::vec3& poleColour,
                   const glm::vec3& flagColour);

    // Convenience helpers
    void buildBox(const glm::vec3& mn, const glm::vec3& mx,
                  const glm::vec3& colour);
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);

    // -------------------------------------------------------------------------
    // CPU-side scratch storage (cleared after upload to GPU).
    // -------------------------------------------------------------------------
    std::vector<Vertex>       mVertices;
    std::vector<unsigned int> mIndices;

    // GL resources
    GLuint  mVAO;
    GLuint  mVBO;
    GLuint  mEBO;
    GLsizei mIndexCount;
    bool    mInitialised;

    // Cached shader program + uniform locations
    GLuint mProgram;
    GLint  mLocMVP;
    GLint  mLocModel;
    GLint  mLocNormalMat;
    GLint  mLocLightDir;
    GLint  mLocLightColor;
    GLint  mLocAmbient;
    GLint  mLocViewPos;

    // Fixed camera framing
    glm::vec3 mCenter;
    float     mRadius;
};