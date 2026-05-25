#pragma once
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
    void drawDepth(GLuint depthShader);
    void cleanup();

    glm::vec3 sceneCenter() const { return mCenter; }
    float     sceneRadius() const { return mRadius; }

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };

    void buildMantle(float rx, float rz, float thick, int segs,
                     const glm::vec3& topColour,
                     const glm::vec3& sideColour);

    void buildMantleKerb(float rx, float rz, int segs,
                         float kerbH, float kerbT,
                         const glm::vec3& colour);

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

    void buildEye(const glm::vec3& centre, float rx, float rz, int segs,
                  const glm::vec3& scleraColour,
                  const glm::vec3& pupilColour);

    void buildTeePatch(const glm::vec3& centre,
                       const glm::vec3& forwardDir,
                       float halfW, float depth,
                       const glm::vec3& colour);

    void buildCup(const glm::vec3& centre, const glm::vec3& colour);
    void buildFlag(const glm::vec3& base, float poleH, float poleR,
                   const glm::vec3& poleColour,
                   const glm::vec3& flagColour);

    void buildBox(const glm::vec3& mn, const glm::vec3& mx,
                  const glm::vec3& colour);
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);

    std::vector<Vertex>       mVertices;
    std::vector<unsigned int> mIndices;

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