#pragma once
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

    glm::vec3 centreline(float s) const;

    glm::vec3 tangent(float s) const;

    void pinchProfile(float s, float& leftHalfW, float& rightHalfW) const;
    void buildFairwayRibbon(int   sSamples,
                            float slabThick,
                            const glm::vec3& colour);

    void buildKerbRibbon(int   sSamples,
                         float kerbH, float kerbT,
                         float baseHalfW,
                         const glm::vec3& colour,
                         bool  leftSide);   // true = left kerb, false = right

    void buildTeePlatform(const glm::vec3& centre,
                          const glm::vec3& tangent,
                          float halfW, float depth,
                          float slabThick,
                          const glm::vec3& colour);

    void buildCup(const glm::vec3& centre, const glm::vec3& colour);
    void buildFlag(const glm::vec3& base, float poleH, float poleR,
                   const glm::vec3& poleColour,
                   const glm::vec3& flagColour);


    void buildEndWall(const glm::vec3& centre,
                      const glm::vec3& segTan,
                      float halfW,
                      float kerbT,
                      float wallT,
                      float wallH);

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