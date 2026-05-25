#pragma once
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


    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };

    void buildFairwaySlab();

    void buildKerbsForSegment(const glm::vec3& startCentre,
                              const glm::vec3& segDir,
                              const glm::vec3& outerPerp,
                              float segLen,
                              float halfW,
                              float kerbH, float kerbT);
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
    void buildEndWall(const glm::vec3& centre,
                      const glm::vec3& segDir,
                      float halfW,
                      float kerbT,
                      float wallT,
                      float wallH);

    // One quad (two triangles) with a flat per-quad normal
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);

    std::vector<Vertex>       mVertices;
    std::vector<unsigned int> mIndices;

    glm::vec3 mTeeEndCentre;
    glm::vec3 mCupEndCentre;

    GLuint  mVAO;
    GLuint  mVBO;
    GLuint  mEBO;
    GLsizei mIndexCount;
    bool    mInitialised;

    GLuint mProgram;
    GLint  mLocMVP;
    GLint  mLocModel;
    GLint  mLocNormalMat;
    GLint  mLocLightDir;
    GLint  mLocLightColor;
    GLint  mLocAmbient;
    GLint  mLocViewPos;

    glm::vec3 mCenter;
    float     mRadius;
};