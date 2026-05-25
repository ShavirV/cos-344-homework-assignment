#include "Hole14.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

namespace
{
    const float START_HEIGHT = 4.0f;
    const float Z_LEN        = 11.0f;
    const float S_AMPLITUDE  = 3.0f;

    const float BASE_HALF_W  = 3.5f;
    const float SLAB_THICK   = 0.2f;

    const float KERB_HEIGHT     = 0.45f;
    const float KERB_THICKNESS  = 0.35f;

    const int   CENTRELINE_SAMPLES = 40;

    const glm::vec3 COLOR_TURF (0.20f, 0.62f, 0.22f);
    const glm::vec3 COLOR_TEE  (0.13f, 0.45f, 0.16f);
    const glm::vec3 COLOR_CUP  (0.04f, 0.04f, 0.04f);
    const glm::vec3 COLOR_KERB (0.72f, 0.71f, 0.69f);
    const glm::vec3 COLOR_POLE (0.95f, 0.95f, 0.95f);
    const glm::vec3 COLOR_FLAG (0.85f, 0.10f, 0.10f);

    const float CUP_RADIUS  = 0.55f;
    const float CUP_DEPTH   = 0.15f;
    const float POLE_HEIGHT = 2.2f;
    const float POLE_RADIUS = 0.06f;

    const float TEE_PLATFORM_DEPTH = 2.5f;
    const float TEE_PLATFORM_HALFW = 3.5f;

    struct Pinch { float s; float sigma; float mag; int side; };
    const Pinch PINCHES[] = {
        { 0.18f, 0.04f, 1.2f, +1 },
        { 0.42f, 0.05f, 1.5f, -1 },
        { 0.62f, 0.04f, 1.0f, +1 },
        { 0.82f, 0.05f, 1.4f, -1 },
    };
    const int NUM_PINCHES = sizeof(PINCHES) / sizeof(PINCHES[0]);

    const glm::vec3 SCENE_CENTER(0.0f, 2.0f, 5.5f);
    const float     SCENE_RADIUS = 18.0f;
}

Hole14::Hole14()
    : mVAO(0), mVBO(0), mEBO(0),
      mIndexCount(0),
      mInitialised(false),
      mProgram(0),
      mLocMVP(-1), mLocModel(-1), mLocNormalMat(-1),
      mLocLightDir(-1), mLocLightColor(-1), mLocAmbient(-1), mLocViewPos(-1),
      mCenter(SCENE_CENTER), mRadius(SCENE_RADIUS)
{
}

Hole14::~Hole14()
{
    cleanup();
}

glm::vec3 Hole14::centreline(float s) const
{
    float x = S_AMPLITUDE * std::sin(2.0f * 3.14159265358979323846f * s);
    float y = START_HEIGHT * (1.0f - s);
    float z = Z_LEN * s;
    return glm::vec3(x, y, z);
}

glm::vec3 Hole14::tangent(float s) const
{
    const float eps = 1e-3f;
    float s1 = std::max(0.0f, s - eps);
    float s2 = std::min(1.0f, s + eps);
    glm::vec3 p1 = centreline(s1);
    glm::vec3 p2 = centreline(s2);
    glm::vec3 t  = p2 - p1;
    float len = glm::length(t);
    if (len < 1e-6f) return glm::vec3(0.0f, 0.0f, 1.0f);
    return t / len;
}

void Hole14::pinchProfile(float s, float& leftHalfW, float& rightHalfW) const
{
    leftHalfW  = BASE_HALF_W;
    rightHalfW = BASE_HALF_W;
    for (int i = 0; i < NUM_PINCHES; ++i)
    {
        const Pinch& p = PINCHES[i];
        float d = s - p.s;
        float bump = p.mag * std::exp(-(d * d) / (2.0f * p.sigma * p.sigma));
        if (p.side >= 0) leftHalfW  -= bump;
        if (p.side <= 0) rightHalfW -= bump;
    }
    const float minHW = 0.8f;
    if (leftHalfW  < minHW) leftHalfW  = minHW;
    if (rightHalfW < minHW) rightHalfW = minHW;
}

void Hole14::pushQuad(const glm::vec3& a, const glm::vec3& b,
                      const glm::vec3& c, const glm::vec3& d,
                      const glm::vec3& n, const glm::vec3& colour)
{
    unsigned int base = (unsigned int)mVertices.size();
    mVertices.push_back({a, n, colour});
    mVertices.push_back({b, n, colour});
    mVertices.push_back({c, n, colour});
    mVertices.push_back({d, n, colour});
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 1);
    mIndices.push_back(base + 2);
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 2);
    mIndices.push_back(base + 3);
}

void Hole14::buildBox(const glm::vec3& mn, const glm::vec3& mx,
                      const glm::vec3& colour)
{
    glm::vec3 p000(mn.x, mn.y, mn.z), p100(mx.x, mn.y, mn.z);
    glm::vec3 p110(mx.x, mx.y, mn.z), p010(mn.x, mx.y, mn.z);
    glm::vec3 p001(mn.x, mn.y, mx.z), p101(mx.x, mn.y, mx.z);
    glm::vec3 p111(mx.x, mx.y, mx.z), p011(mn.x, mx.y, mx.z);
    pushQuad(p001, p101, p111, p011, { 0, 0,  1}, colour);
    pushQuad(p100, p000, p010, p110, { 0, 0, -1}, colour);
    pushQuad(p000, p001, p011, p010, {-1, 0,  0}, colour);
    pushQuad(p101, p100, p110, p111, { 1, 0,  0}, colour);
    pushQuad(p011, p111, p110, p010, { 0, 1,  0}, colour);
    pushQuad(p000, p100, p101, p001, { 0,-1,  0}, colour);
}

void Hole14::buildFairwayRibbon(int   sSamples,
                                float slabThick,
                                const glm::vec3& colour)
{
    int N = sSamples;
    std::vector<glm::vec3> leftTop (N + 1), rightTop (N + 1);
    std::vector<glm::vec3> leftBot (N + 1), rightBot (N + 1);
    std::vector<glm::vec3> topNorm (N + 1);
    std::vector<glm::vec3> sideVec (N + 1);

    for (int i = 0; i <= N; ++i)
    {
        float s = (float)i / (float)N;
        glm::vec3 c = centreline(s);
        glm::vec3 t = tangent(s);
        glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
        float sideLen = glm::length(side);
        if (sideLen < 1e-6f)
            side = glm::vec3(1.0f, 0.0f, 0.0f);
        else
            side /= sideLen;

        glm::vec3 up = glm::cross(side, t);
        if (up.y < 0.0f) up = -up;

        float leftHW, rightHW;
        pinchProfile(s, leftHW, rightHW);

        leftTop [i] = c + side * leftHW;
        rightTop[i] = c - side * rightHW;

        leftBot [i] = leftTop [i] - glm::vec3(0.0f, slabThick, 0.0f);
        rightBot[i] = rightTop[i] - glm::vec3(0.0f, slabThick, 0.0f);

        topNorm[i] = up;
        sideVec[i] = side;
    }

    for (int i = 0; i < N; ++i)
    {
        glm::vec3 nTop = glm::normalize(topNorm[i] + topNorm[i + 1]);
        pushQuad(leftTop[i], rightTop[i], rightTop[i + 1], leftTop[i + 1],
                 nTop, colour);

        pushQuad(leftBot[i], leftBot[i + 1], rightBot[i + 1], rightBot[i],
                 -nTop, colour);

        glm::vec3 nSide = glm::normalize(sideVec[i] + sideVec[i + 1]);
        pushQuad(leftBot[i], leftBot[i + 1], leftTop[i + 1], leftTop[i],
                 nSide, colour);

        pushQuad(rightBot[i + 1], rightBot[i], rightTop[i], rightTop[i + 1],
                 -nSide, colour);
    }

    {
        glm::vec3 t0 = tangent(0.0f);
        pushQuad(rightBot[0], leftBot[0], leftTop[0], rightTop[0],
                 -t0, colour);
    }
    {
        glm::vec3 tN = tangent(1.0f);
        pushQuad(leftBot[N], rightBot[N], rightTop[N], leftTop[N],
                 tN, colour);
    }
}

void Hole14::buildKerbRibbon(int   sSamples,
                             float kerbH, float kerbT,
                             float baseHalfW,
                             const glm::vec3& colour,
                             bool  leftSide)
{
    int N = sSamples;
    std::vector<glm::vec3> innerBot(N + 1), outerBot(N + 1);
    std::vector<glm::vec3> innerTop(N + 1), outerTop(N + 1);
    std::vector<glm::vec3> sideVec(N + 1);

    for (int i = 0; i <= N; ++i)
    {
        float s = (float)i / (float)N;
        glm::vec3 c = centreline(s);
        glm::vec3 t = tangent(s);
        glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
        float sLen = glm::length(side);
        if (sLen < 1e-6f) side = glm::vec3(1.0f, 0.0f, 0.0f);
        else              side /= sLen;

        float leftHW, rightHW;
        pinchProfile(s, leftHW, rightHW);

        if (leftSide)
        {
            innerBot[i] = c + side * leftHW;
            outerBot[i] = c + side * (baseHalfW + kerbT);
        }
        else
        {
            innerBot[i] = c - side * rightHW;
            outerBot[i] = c - side * (baseHalfW + kerbT);
        }
        innerTop[i] = innerBot[i] + glm::vec3(0.0f, kerbH, 0.0f);
        outerTop[i] = outerBot[i] + glm::vec3(0.0f, kerbH, 0.0f);

        sideVec[i] = side;
    }

    glm::vec3 outDirSign(leftSide ? 1.0f : -1.0f);

    for (int i = 0; i < N; ++i)
    {
        glm::vec3 sideAvg = glm::normalize(sideVec[i] + sideVec[i + 1]);
        glm::vec3 outN = sideAvg * (leftSide ? 1.0f : -1.0f);

        if (leftSide)
        {
            pushQuad(outerTop[i], outerTop[i + 1], innerTop[i + 1], innerTop[i],
                     glm::vec3(0, 1, 0), colour);
        }
        else
        {
            pushQuad(innerTop[i], innerTop[i + 1], outerTop[i + 1], outerTop[i],
                     glm::vec3(0, 1, 0), colour);
        }

        if (leftSide)
        {
            pushQuad(outerBot[i], outerBot[i + 1], outerTop[i + 1], outerTop[i],
                     outN, colour);
        }
        else
        {
            pushQuad(outerBot[i + 1], outerBot[i], outerTop[i], outerTop[i + 1],
                     outN, colour);
        }

        if (leftSide)
        {
            pushQuad(innerBot[i + 1], innerBot[i], innerTop[i], innerTop[i + 1],
                     -outN, colour);
        }
        else
        {
            pushQuad(innerBot[i], innerBot[i + 1], innerTop[i + 1], innerTop[i],
                     -outN, colour);
        }
    }

    glm::vec3 t0 = tangent(0.0f), tN = tangent(1.0f);
    if (leftSide)
    {
        pushQuad(innerBot[0], outerBot[0], outerTop[0], innerTop[0],
                 -t0, colour);
        pushQuad(outerBot[N], innerBot[N], innerTop[N], outerTop[N],
                 tN, colour);
    }
    else
    {
        pushQuad(outerBot[0], innerBot[0], innerTop[0], outerTop[0],
                 -t0, colour);
        pushQuad(innerBot[N], outerBot[N], outerTop[N], innerTop[N],
                 tN, colour);
    }
}

void Hole14::buildTeePlatform(const glm::vec3& centre,
                              const glm::vec3& tan,
                              float halfW, float depth,
                              float slabThick,
                              const glm::vec3& colour)
{
    glm::vec3 t = glm::normalize(glm::vec3(tan.x, 0.0f, tan.z));
    if (glm::length(t) < 1e-6f) t = glm::vec3(0, 0, 1);
    glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(side) < 1e-6f) side = glm::vec3(1, 0, 0);
    else side = glm::normalize(side);

    glm::vec3 back = centre - t * depth;

    glm::vec3 frontL = centre + side * halfW;
    glm::vec3 frontR = centre - side * halfW;
    glm::vec3 backL  = back   + side * halfW;
    glm::vec3 backR  = back   - side * halfW;

    glm::vec3 dn(0.0f, slabThick, 0.0f);
    glm::vec3 frontLb = frontL - dn, frontRb = frontR - dn;
    glm::vec3 backLb  = backL  - dn, backRb  = backR  - dn;

    pushQuad(backL, backR, frontR, frontL, glm::vec3(0, 1, 0), colour);
    pushQuad(backLb, frontLb, frontRb, backRb, glm::vec3(0,-1, 0), colour);
    pushQuad(backLb, backL, frontL, frontLb, side, colour);
    pushQuad(frontRb, frontR, backR, backRb, -side, colour);
    pushQuad(backRb, backR, backL, backLb, -t, colour);
    pushQuad(frontLb, frontL, frontR, frontRb, t, colour);
}

void Hole14::buildEndWall(const glm::vec3& centre,
                          const glm::vec3& segTan,
                          float halfW,
                          float kerbT,
                          float wallT,
                          float wallH)
{
    glm::vec3 t    = glm::normalize(segTan);
    glm::vec3 side = glm::cross(t, glm::vec3(0.0f, 1.0f, 0.0f));
    float sLen = glm::length(side);
    if (sLen < 1e-6f) side = glm::vec3(1.0f, 0.0f, 0.0f);
    else              side /= sLen;

    float span  = halfW + kerbT;
    float baseY = centre.y;
    float topY  = baseY + wallH;

    glm::vec3 BL  = centre + span * side;
    glm::vec3 BR  = centre - span * side;
    glm::vec3 BLf = BL + wallT * t;
    glm::vec3 BRf = BR + wallT * t;

    glm::vec3 TL  (BL.x,  topY, BL.z);
    glm::vec3 TR  (BR.x,  topY, BR.z);
    glm::vec3 TLf (BLf.x, topY, BLf.z);
    glm::vec3 TRf (BRf.x, topY, BRf.z);

    pushQuad(BR, BL, TL, TR, -t, COLOR_KERB);
    pushQuad(BLf, BRf, TRf, TLf, t, COLOR_KERB);
    pushQuad(TL, TR, TRf, TLf, glm::vec3(0, 1, 0), COLOR_KERB);
    pushQuad(BLf, BL, TL, TLf, side, COLOR_KERB);
    pushQuad(BR, BRf, TRf, TR, -side, COLOR_KERB);
    pushQuad(BL, BLf, BRf, BR, glm::vec3(0, -1, 0), COLOR_KERB);
}

void Hole14::buildCup(const glm::vec3& centre, const glm::vec3& colour)
{
    glm::vec3 mn(centre.x - CUP_RADIUS, centre.y - CUP_DEPTH, centre.z - CUP_RADIUS);
    glm::vec3 mx(centre.x + CUP_RADIUS, centre.y + 0.002f,    centre.z + CUP_RADIUS);
    buildBox(mn, mx, colour);
}

void Hole14::buildFlag(const glm::vec3& base, float poleH, float poleR,
                       const glm::vec3& poleColour,
                       const glm::vec3& flagColour)
{
    glm::vec3 mn(base.x - poleR, base.y,         base.z - poleR);
    glm::vec3 mx(base.x + poleR, base.y + poleH, base.z + poleR);
    buildBox(mn, mx, poleColour);

    float flagW = 0.9f;
    float flagH = 0.55f;
    glm::vec3 fA(base.x,         base.y + poleH - flagH,        base.z);
    glm::vec3 fB(base.x + flagW, base.y + poleH - flagH * 0.5f, base.z);
    glm::vec3 fC(base.x,         base.y + poleH,                base.z);

    {
        unsigned int idx = (unsigned int)mVertices.size();
        mVertices.push_back({fA, { 0, 0,  1}, flagColour});
        mVertices.push_back({fB, { 0, 0,  1}, flagColour});
        mVertices.push_back({fC, { 0, 0,  1}, flagColour});
        mIndices.push_back(idx + 0);
        mIndices.push_back(idx + 1);
        mIndices.push_back(idx + 2);
    }
    {
        unsigned int idx = (unsigned int)mVertices.size();
        mVertices.push_back({fA, { 0, 0, -1}, flagColour});
        mVertices.push_back({fC, { 0, 0, -1}, flagColour});
        mVertices.push_back({fB, { 0, 0, -1}, flagColour});
        mIndices.push_back(idx + 0);
        mIndices.push_back(idx + 1);
        mIndices.push_back(idx + 2);
    }
}

void Hole14::init(GLuint shaderProgram)
{
    if (mInitialised) return;

    mProgram = shaderProgram;

    mLocModel      = glGetUniformLocation(mProgram, "uModel");
    mLocNormalMat  = glGetUniformLocation(mProgram, "uNormalMat");
    mLocLightDir   = glGetUniformLocation(mProgram, "uLightDir");
    mLocViewPos    = glGetUniformLocation(mProgram, "uViewPos");

    mVertices.clear();
    mIndices.clear();

    buildFairwayRibbon(CENTRELINE_SAMPLES, SLAB_THICK, COLOR_TURF);

    buildKerbRibbon(CENTRELINE_SAMPLES, KERB_HEIGHT, KERB_THICKNESS,
                    BASE_HALF_W, COLOR_KERB, /*leftSide=*/true);
    buildKerbRibbon(CENTRELINE_SAMPLES, KERB_HEIGHT, KERB_THICKNESS,
                    BASE_HALF_W, COLOR_KERB, /*leftSide=*/false);

    {
        glm::vec3 teeCentre = centreline(0.0f);
        glm::vec3 teeTan    = tangent(0.0f);
        buildTeePlatform(teeCentre, teeTan,
                         TEE_PLATFORM_HALFW, TEE_PLATFORM_DEPTH,
                         SLAB_THICK, COLOR_TURF);

        const float eps = 0.001f;
        glm::vec3 mn(teeCentre.x - 1.2f, teeCentre.y + eps,         teeCentre.z - 1.2f);
        glm::vec3 mx(teeCentre.x + 1.2f, teeCentre.y + 0.02f + eps, teeCentre.z + 1.2f);
        buildBox(mn, mx, COLOR_TEE);
    }

    {
        glm::vec3 cupEnd = centreline(1.0f);
        glm::vec3 cupTan = tangent(1.0f);
        const float CUP_WALL_CLEARANCE = 0.3f;
        glm::vec3 cupCentre = cupEnd - (CUP_RADIUS + CUP_WALL_CLEARANCE) * cupTan;

        buildCup(cupCentre, COLOR_CUP);
        glm::vec3 poleBase(cupCentre.x, cupCentre.y, cupCentre.z);
        buildFlag(poleBase, POLE_HEIGHT, POLE_RADIUS,
                  COLOR_POLE, COLOR_FLAG);
    }

    {
        glm::vec3 cupCentre = centreline(1.0f);
        glm::vec3 cupTan    = tangent(1.0f);
        buildEndWall(cupCentre,
                     cupTan,
                     BASE_HALF_W,
                     KERB_THICKNESS,
                     /*wallT=*/KERB_THICKNESS,
                     /*wallH=*/KERB_HEIGHT);
    }

    if (mVertices.empty() || mIndices.empty())
    {
        std::cerr << "Hole14::init — no geometry was generated!" << std::endl;
        return;
    }

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 mVertices.size() * sizeof(Vertex),
                 mVertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mIndices.size() * sizeof(unsigned int),
                 mIndices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mIndexCount = (GLsizei)mIndices.size();

    mCenter = SCENE_CENTER;
    mRadius = SCENE_RADIUS;

    mVertices.clear(); mVertices.shrink_to_fit();
    mIndices.clear();  mIndices.shrink_to_fit();

    mInitialised = true;
}

void Hole14::draw(const glm::mat4& view,
                  const glm::mat4& proj,
                  const glm::vec3& sunDir,
                  const glm::vec3& camPos)
{
    if (!mInitialised) 
        return;

    glUseProgram(mProgram);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(150.0f, 0.35f, -15.0f));
    glm::mat3 nrmM  = glm::mat3(glm::transpose(glm::inverse(model)));

    if (mLocModel      != -1) 
        glUniformMatrix4fv(mLocModel,1, GL_FALSE, glm::value_ptr(model));
    if (mLocNormalMat  != -1) 
        glUniformMatrix3fv(mLocNormalMat, 1, GL_FALSE, glm::value_ptr(nrmM));
    if (mLocLightDir   != -1)
        glUniform3fv(mLocLightDir,1, glm::value_ptr(sunDir));
    if (mLocViewPos    != -1) 
        glUniform3fv(mLocViewPos, 1, glm::value_ptr(camPos));

    (void)view; (void)proj;

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Hole14::drawDepth(GLuint depthShader)
{
    if (!mInitialised) 
        return;

    GLint loc_uModel = glGetUniformLocation(depthShader, "uModel");
    if (loc_uModel != -1) 
    {
        glm::mat4 model(1.0f);
        glUniformMatrix4fv(loc_uModel, 1, GL_FALSE, glm::value_ptr(model));
    }

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Hole14::cleanup()
{
    if (mEBO) { glDeleteBuffers(1, &mEBO); mEBO = 0; }
    if (mVBO) { glDeleteBuffers(1, &mVBO); mVBO = 0; }
    if (mVAO) { glDeleteVertexArrays(1, &mVAO); mVAO = 0; }
    mIndexCount = 0;
    mInitialised = false;
}