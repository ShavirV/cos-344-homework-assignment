#pragma once
// =============================================================================
// Hole15.h
// Hole 15 of the Dino Park miniature golf course — the Octopus Head.
//
// Geometry
// --------
// The hole is styled after the top-down silhouette of an octopus head:
//
//   • A large elliptical "mantle" slab forms the body of the head — the
//     central fairway area the ball must traverse.
//
//   • Eight curved tentacle corridors radiate outward from the mantle in
//     a fan arrangement. Seven are dead-end traps; the eighth is the live
//     playable path. Tentacles curl slightly at their tips (≈ 40° arc) to
//     give the organic, floppy silhouette typical of a cephalopod.
//
//   • The tee sits at the far RIGHT of the mantle (east side). The cup is
//     set into the far LEFT (west side). Concrete kerbs run the full length
//     of every corridor and around the perimeter of the mantle, so wayward
//     shots are clearly corralled.
//
//   • Two small "eye" patches (dark sclera + white pupil highlight boxes)
//     are embedded in the top face of the mantle as decorative landmarks.
//
// Playable path
// -------------
// The ball starts on the tee (right), rolls into the mantle, and must exit
// through the CORRECT tentacle (the bottom-right one — tentacle index 6 in
// the 0-based fan) to reach the cup. All other tentacles dead-end in a
// curved wall — a ball that enters them returns to the mantle via the kerb
// bounce or must be replayed. (Dead-end geometry is purely visual; game
// logic is not implemented here.)
//
// Coordinate system
// -----------------
//   +X right,  +Y up,  +Z into screen (right-hand, Y-up).
//   Mantle centre is at the world origin.  Tee at ( +MANTLE_RX + 0.5, 0, 0).
//   Cup at ( -MANTLE_RX - 0.3, 0, 0 ).
//   Tentacle fan spans ≈ -120° … +120° from the -Z axis on the near side,
//   plus one live tentacle on the right and one on the left as entry/exit.
//
// Interface
// ---------
// Identical to Hole13 / Hole14:
//   init(program)   — build + upload geometry, cache uniform locations
//   draw(...)       — issue the GL draw call
//   cleanup()       — free GL resources
//   sceneCenter()   — camera framing hint
//   sceneRadius()   — camera framing hint
// =============================================================================

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Hole15
{
public:
    Hole15();
    ~Hole15();

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
    // Vertex layout — identical to Hole13/14 (aPos=0, aNormal=1, aColor=2).
    // -------------------------------------------------------------------------
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };

    // =========================================================================
    // Geometry builders — each appends to mVertices / mIndices.
    // =========================================================================

    // Large elliptical mantle slab (the octopus "head").
    //   rx, rz  — half-extents of the ellipse in X and Z
    //   thick   — slab thickness (downward from y = 0)
    //   segs    — number of angular segments (higher → smoother ellipse)
    //   colour  — turf green for top face; slightly darker for sides
    void buildMantle(float rx, float rz, float thick, int segs,
                     const glm::vec3& topColour,
                     const glm::vec3& sideColour);

    // Kerb strip around the top perimeter of the mantle ellipse.
    //   rx, rz  — same ellipse radii as buildMantle
    //   kerbH, kerbT — kerb height above slab top, kerb outward thickness
    void buildMantleKerb(float rx, float rz, int segs,
                         float kerbH, float kerbT,
                         const glm::vec3& colour);

    // One tentacle: a short straight corridor + a curling arc at the tip.
    //   rootPt  — point on the mantle ellipse perimeter where the tentacle
    //             originates (at y = 0, top of slab)
    //   outDir  — unit outward direction at rootPt (normal to ellipse)
    //   sideDir — unit direction perpendicular to outDir, in XZ plane
    //   halfW   — half-width of the tentacle corridor
    //   length  — straight-run length before the curl begins
    //   curlR   — radius of the tip curl arc
    //   curlAng — total curl angle in radians (positive = curl left wrt outDir)
    //   thick   — slab thickness
    //   arcSegs — number of quads in the curl arc
    //   colour  — turf green
    void buildTentacle(const glm::vec3& rootPt,
                       const glm::vec3& outDir,
                       const glm::vec3& sideDir,
                       float halfW,
                       float length,
                       float curlR,
                       float curlAng,
                       float thick,
                       int   arcSegs,
                       const glm::vec3& colour);

    // Kerb pair for one tentacle (matches buildTentacle geometry).
    void buildTentacleKerb(const glm::vec3& rootPt,
                           const glm::vec3& outDir,
                           const glm::vec3& sideDir,
                           float halfW,
                           float length,
                           float curlR,
                           float curlAng,
                           int   arcSegs,
                           float kerbH, float kerbT,
                           const glm::vec3& colour);

    // Decorative eye inset on the mantle top face.
    //   centre  — world position (y should sit just above the slab top)
    //   rx, rz  — ellipse radii of the eye
    //   segs    — angular subdivisions
    void buildEye(const glm::vec3& centre, float rx, float rz, int segs,
                  const glm::vec3& scleraColour,
                  const glm::vec3& pupilColour);

    // Tee patch (dark green rectangle at the entry point).
    void buildTeePatch(const glm::vec3& centre,
                       const glm::vec3& forwardDir,
                       float halfW, float depth,
                       const glm::vec3& colour);

    // Cup + flag at the exit point.
    void buildCup(const glm::vec3& centre, const glm::vec3& colour);
    void buildFlag(const glm::vec3& base, float poleH, float poleR,
                   const glm::vec3& poleColour,
                   const glm::vec3& flagColour);

    // General helpers (same signatures as Hole13/14).
    void buildBox(const glm::vec3& mn, const glm::vec3& mx,
                  const glm::vec3& colour);
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);

    // -------------------------------------------------------------------------
    // CPU-side scratch storage (cleared after GPU upload).
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