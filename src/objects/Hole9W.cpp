#include "Hole9W.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

static const float HOLE_W = 28.0f;
static const float HOLE_D = 15.0f;
static const float HALF_W = 14.0f;
static const float HALF_D =  7.5f;

static const float FH = 0.20f;
static const float BT = 0.5f;
static const float BH = 0.4f;

static const float GR = 0.20f, GG = 0.62f, GB = 0.22f;
static const float MR = 0.28f, MG = 0.20f, MB = 0.10f;
static const float CR = 0.86f, CG = 0.86f, CB = 0.86f;
static const float TUNNEL_R = 0.22f, TUNNEL_G = 0.14f, TUNNEL_B = 0.05f;

static const float MARGIN  = 1.2f;
static const float INNER_X = HALF_W - MARGIN;
static const float INNER_Z = HALF_D - MARGIN;

void Hole9W::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole9W::addFlagAt(float x, float baseY, float z)
{
    Mesh* pole = new Mesh(Mesh::makeCylinder(0.10f, 4.5f, 8,
                                              0.85f, 0.85f, 0.85f, 1.0f));
    pole->translate(-x, baseY + 2.25f, z);
    pole->setup(); addMesh(pole);

    Mesh* pennant = new Mesh(Mesh::makeTriangularPrism(1.6f, 0.85f, 0.12f,
                                                        0.9f, 0.08f, 0.08f, 1.0f));
    pennant->translate(-x - 0.8f, baseY + 4.4f, z);
    pennant->setup(); addMesh(pennant);
}

void Hole9W::addFlowerAt(float x, float baseY, float z,
                          float pr, float pg, float pb)
{
    Mesh* stem = new Mesh(Mesh::makeCylinder(0.055f, 0.55f, 6,
                                              0.2f, 0.55f, 0.1f, 1.0f));
    stem->translate(-x, baseY + 0.275f, z);
    stem->setup(); addMesh(stem);

    Mesh* petal = new Mesh(Mesh::makeCone(0.32f, 0.48f, 8, pr, pg, pb, 1.0f));
    petal->translate(-x, baseY + 0.70f, z);
    petal->setup(); addMesh(petal);

    Mesh* centre = new Mesh(Mesh::makeCylinder(0.11f, 0.12f, 8,
                                                1.0f, 0.88f, 0.1f, 1.0f));
    centre->translate(-x, baseY + 0.98f, z);
    centre->setup(); addMesh(centre);
}

void Hole9W::addTree(float x, float baseY, float z, float s)
{
    Mesh* trunk = new Mesh(Mesh::makeCylinder(0.32f * s, 4.0f * s, 8,
                                               0.44f, 0.29f, 0.11f, 1.0f));
    trunk->translate(-x, baseY + 2.0f * s, z);
    trunk->setup(); addMesh(trunk);

    Mesh* c1 = new Mesh(Mesh::makeCone(2.4f * s, 3.2f * s, 10,
                                        0.10f, 0.44f, 0.14f, 1.0f));
    c1->translate(-x, baseY + 5.0f * s, z);
    c1->setup(); addMesh(c1);

    Mesh* c2 = new Mesh(Mesh::makeCone(1.5f * s, 2.2f * s, 10,
                                        0.12f, 0.50f, 0.18f, 1.0f));
    c2->translate(-x, baseY + 7.0f * s, z);
    c2->setup(); addMesh(c2);
}

Hole9W::Hole9W(float s)
    : meshCount(0), scale(s)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    buildFairway();
    //buildTunnel();
    buildRocks();
    buildFlag();
    buildTrees();
    buildFlowers();
}

Hole9W::~Hole9W()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole9W::buildFairway()
{
    Mesh* fairway = new Mesh(Mesh::makeCuboid(HOLE_W, FH, HOLE_D,
                                               GR, GG, GB, 1.0f));
    fairway->translate(0.0f, FH * 0.5f, 0.0f);
    fairway->setup(); addMesh(fairway);

    Mesh* earth = new Mesh(Mesh::makeCuboid(HOLE_W, 0.5f, HOLE_D,
                                             MR, MG, MB, 1.0f));
    earth->translate(0.0f, -0.25f, 0.0f);
    earth->setup(); addMesh(earth);

    float wallXNeg = -HALF_W - BT * 0.5f;
    float wallXPos =  HALF_W + BT * 0.5f;
    float wallZNeg = -HALF_D - BT * 0.5f;
    float wallZPos =  HALF_D + BT * 0.5f;

    Mesh* bZPos = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CR, CG, CB, 1.0f));
    bZPos->translate(0.0f, BH * 0.5f, wallZPos);
    bZPos->setup(); addMesh(bZPos);

    Mesh* bZNeg = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CR, CG, CB, 1.0f));
    bZNeg->translate(0.0f, BH * 0.5f, wallZNeg);
    bZNeg->setup(); addMesh(bZNeg);

    // +X border: full wall (green/flag end, mirrored)
    Mesh* bXPos = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT,
                                             CR, CG, CB, 1.0f));
    bXPos->translate(wallXPos, BH * 0.5f, 0.0f);
    bXPos->setup(); addMesh(bXPos);

    // -X border: gap for the tunnel mouth (mirrored from +X)
    const float tunnelHalfZ = 3.6f;
    const float wallSegD    = (HOLE_D - 2.0f * tunnelHalfZ) * 0.5f;
    const float wallSegCZ   = tunnelHalfZ + wallSegD * 0.5f;

    Mesh* bXNeg_a = new Mesh(Mesh::makeCuboid(BT, BH, wallSegD, CR, CG, CB, 1.0f));
    bXNeg_a->translate(wallXNeg, BH * 0.5f,  wallSegCZ);
    bXNeg_a->setup(); addMesh(bXNeg_a);

    Mesh* bXNeg_b = new Mesh(Mesh::makeCuboid(BT, BH, wallSegD, CR, CG, CB, 1.0f));
    bXNeg_b->translate(wallXNeg, BH * 0.5f, -wallSegCZ);
    bXNeg_b->setup(); addMesh(bXNeg_b);
}

void Hole9W::buildTunnel()
{
    Mesh* mouth = new Mesh(Mesh::makeTunnel(
        3.5f,
        2.5f,
        2.0f,
        32,
        TUNNEL_R, TUNNEL_G, TUNNEL_B, 1.0f));

    mouth->rotate(90.0f, 0.0f, 0.0f, 1.0f);
    mouth->translate(-HALF_W + 1.0f, 3.5f, 0.0f);
    mouth->setup(); addMesh(mouth);
}

void Hole9W::buildRocks()
{
    // Two central obstacles (X negated)
    Mesh* rock1 = new Mesh(Mesh::makeRock(1.0f));
    rock1->translate(-6.0f, 1.0f, 4.0f);
    rock1->setup(); addMesh(rock1);

    Mesh* rock2 = new Mesh(Mesh::makeRock(1.5f));
    rock2->translate(0.0f, 1.5f, 0.0f);
    rock2->setup(); addMesh(rock2);

    struct RockDef { float x, z, s; };
    RockDef rocks[] = {
        // -Z border
        {  10.0f, -(INNER_Z - 0.5f), 0.72f },
        {   3.0f, -(INNER_Z - 0.6f), 0.66f },
        {  -4.0f, -(INNER_Z - 0.4f), 0.78f },
        { -10.5f, -(INNER_Z - 0.5f), 0.74f },

        // +Z border
        {   9.5f,  (INNER_Z - 0.6f), 0.70f },
        {   3.5f,  (INNER_Z - 0.5f), 0.76f },
        { -10.0f,  (INNER_Z - 0.4f), 0.68f },

        // +X end corner accents (mirrored from -X)
        {  12.5f, -5.5f, 0.62f },
        {  12.5f,  5.5f, 0.60f },
    };

    for (auto& rd : rocks)
    {
        Mesh* r = new Mesh(Mesh::makeRock(rd.s));
        r->translate(rd.x, rd.s, rd.z);
        r->setup(); addMesh(r);
    }
}

void Hole9W::buildFlag()
{
    float flagX =  8.0f;
    float flagZ =  0.0f;

    Mesh* cup = new Mesh(Mesh::makeCylinder(0.48f, 0.28f, 12,
                                             0.10f, 0.10f, 0.10f, 1.0f));
    cup->translate(-flagX, FH + 0.04f, flagZ);
    cup->setup(); addMesh(cup);

    addFlagAt(-flagX, FH, flagZ);
}

void Hole9W::buildTrees()
{
    struct TreeDef { float x, z, s; };
    TreeDef trees[] = {
        // -Z border (skip near tunnel mouth at -HALF_W)
        {  13.0f, -6.0f, 0.85f },
        {   7.0f, -6.0f, 0.80f },
        {   0.0f, -6.0f, 0.78f },
        {  -7.0f, -6.0f, 0.82f },

        // +Z border
        {  13.0f,  6.0f, 0.85f },
        {   7.0f,  6.0f, 0.78f },
        {   0.0f,  6.0f, 0.82f },
        {  -7.0f,  6.0f, 0.80f },
    };

    for (auto& td : trees)
        addTree(td.x, FH, td.z, td.s);
}

void Hole9W::buildFlowers()
{
    float baseY = FH;

    struct FlowerDef { float x, z; float r, g, b; };
    FlowerDef flowers[] = {
        {  12.5f, -3.0f, 1.00f, 0.75f, 0.10f },
        {   4.0f, -4.5f, 0.85f, 0.20f, 0.80f },
        {  -2.5f, -4.5f, 0.95f, 0.30f, 0.60f },
        {  -9.0f, -4.5f, 1.00f, 0.40f, 0.10f },

        {  12.5f,  3.0f, 0.90f, 0.85f, 0.10f },
        {   4.0f,  4.5f, 0.80f, 0.25f, 0.70f },
        {  -2.5f,  4.5f, 0.95f, 0.88f, 0.10f },
        {  -9.0f,  4.5f, 1.00f, 0.35f, 0.55f },

        // Near the flag
        {   9.5f,  2.0f, 0.95f, 0.88f, 0.10f },
        {   6.5f, -2.0f, 1.00f, 0.35f, 0.55f },
    };

    for (auto& fd : flowers)
        addFlowerAt(fd.x, baseY, fd.z, fd.r, fd.g, fd.b);
}

void Hole9W::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole9W::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}

void Hole9W::draw(GLuint shader, bool wireframe,
                  const glm::vec3& lightDir,
                  const glm::vec3& lightColor,
                  const glm::vec3& camPos)
{
    glUseProgram(shader);
    GLint locLD = glGetUniformLocation(shader, "uLightDir");
    GLint locVP = glGetUniformLocation(shader, "uViewPos");
    GLint locLC = glGetUniformLocation(shader, "uLightColor");
    if (locLD != -1) glUniform3fv(locLD, 1, glm::value_ptr(glm::normalize(lightDir)));
    if (locVP != -1) glUniform3fv(locVP, 1, glm::value_ptr(camPos));
    if (locLC != -1) glUniform3fv(locLC, 1, glm::value_ptr(lightColor));

    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shader, wireframe);
}

void Hole9W::drawDepth(GLuint depthShader)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->drawDepth(depthShader);
}