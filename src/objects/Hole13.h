#pragma once
// =============================================================================
// Hole13.h
// Hole 13 of the Dino Park miniature golf course — the boomerang hole.
//
// Geometry
// --------
// A dogleg fairway with a 120° interior angle at the bend (halfway along).
// Two equal-length straight segments meet at the bend with a mitred corner,
// concrete kerbs run along both edges of each segment, a darker tee patch
// sits at one end, and a black cup with a flag-pole + pennant sits at the
// other end. No surrounding terrain — just the hole itself.
//
// Interface matches dev.cpp's expectations:
//   init(program)     — uploads geometry to GPU, caches uniform locations
//   draw(...)         — issues the GL draw call with all required uniforms
//   cleanup()         — releases all GL resources
//   sceneCenter()     — fixed value used by the dev viewer to frame the camera
//   sceneRadius()     — fixed value used by the dev viewer to frame the camera
//
// Only spec-allowed libraries (GL/glew.h, glm) and standard C++ are used.
// =============================================================================

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Hole13
{
public:
    Hole13();
    ~Hole13();

    void init(GLuint shaderProgram);
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& sunDir,
              const glm::vec3& camPos);
    void drawDepth(GLuint depthShader);
    void cleanup();

    glm::vec3 sceneCenter() const { return mCenter; }
    float     sceneRadius() const { return mRadius; }

private:
    // -------------------------------------------------------------------------
    // Vertex layout — must match dev.cpp's shader (aPos=0, aNormal=1, aColor=2)
    // -------------------------------------------------------------------------
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };

    // -------------------------------------------------------------------------
    // CPU-side build helpers. Each appends triangles to mVertices / mIndices.
    // -------------------------------------------------------------------------

    // Build the fairway slab as a hexagonal top + matching bottom + sides,
    // with the two segments mitred at the bend. Populates mTeeEndCentre /
    // mCupEndCentre as a side-effect so the tee patch and cup can be placed
    // without re-deriving the geometry.
    void buildFairwaySlab();

    // Build the two kerbs (outer + inner) for one fairway segment, sitting on
    // top of the fairway slab.
    //   startCentre — centreline point where the straight segment begins
    //                 (segment-side end of the kerb; with a filleted bend
    //                 this is the tangent point, not the bend itself)
    //   segDir     — unit vector from startCentre toward the segment's far end
    //   outerPerp  — unit vector perpendicular to segDir, pointing to the
    //                "outer" edge of this segment (away from the V's inside)
    //   segLen     — length of the straight portion of the segment
    //   halfW      — half-width of the fairway
    //   kerbH, kerbT — kerb height and thickness
    void buildKerbsForSegment(const glm::vec3& startCentre,
                              const glm::vec3& segDir,
                              const glm::vec3& outerPerp,
                              float segLen,
                              float halfW,
                              float kerbH, float kerbT);

    // Build the two kerbs (outer + inner) following the fillet arc at the
    // bend. The arc is centred at `arcCentre` and has fairway-centreline
    // radius `arcR`; the fairway edges are at `arcR ± halfW`. Sweeps from
    // `angStart` to `angEnd` (radians, atan2 convention in the XZ plane),
    // tessellated into `segments` quads to match the fairway-slab arc.
    void buildArcKerbs(const glm::vec3& arcCentre,
                       float arcR,
                       float halfW,
                       float angStart, float angEnd,
                       int   segments,
                       float kerbH, float kerbT);

    // Axis-aligned box helper — used for the tee patch, the cup, and the pole.
    void buildBox(const glm::vec3& mn, const glm::vec3& mx,
                  const glm::vec3& colour);

    // Flag pole + triangular pennant.
    void buildFlag(const glm::vec3& base, float poleH, float poleR,
                   const glm::vec3& poleColour,
                   const glm::vec3& flagColour);

    // Closing boundary wall at the cup end — a kerb-height slab that spans the
    // full fairway width (plus kerb thickness on each side) so the hole doesn't
    // just open into empty space.
    //   centre   — centre of the fairway end-line (mCupEndCentre)
    //   segDir   — unit vector pointing FROM bend TOWARD the cup end
    //   halfW    — fairway half-width
    //   kerbT    — kerb thickness (wall extends this far beyond each edge)
    //   wallT    — wall thickness along segDir
    //   wallH    — wall height above fairway top
    void buildEndWall(const glm::vec3& centre,
                      const glm::vec3& segDir,
                      float halfW,
                      float kerbT,
                      float wallT,
                      float wallH);

    // One quad (two triangles) with a flat per-quad normal.
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);

    // -------------------------------------------------------------------------
    // CPU-side scratch storage (cleared after upload to GPU).
    // -------------------------------------------------------------------------
    std::vector<Vertex>       mVertices;
    std::vector<unsigned int> mIndices;

    // Computed by buildFairwaySlab(), consumed by init() for tee patch / cup
    // / flag placement.
    glm::vec3 mTeeEndCentre;
    glm::vec3 mCupEndCentre;

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

    // Fixed camera framing (hardcoded — no AABB tracking).
    glm::vec3 mCenter;
    float     mRadius;
};