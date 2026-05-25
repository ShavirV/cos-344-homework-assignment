#include "Hole8.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

static const float HOLE_W = 28.0f;
static const float HOLE_D = 15.0f;
static const float HALF_W = 14.0f;
static const float HALF_D =  7.5f;

// Slab / kerb
static const float FH = 0.18f;   // fairway slab thickness
static const float BT = 0.5f;    // border wall thickness
static const float BH = 0.4f;    // border kerb height

// 7 evenly-spaced steps. Step width = 28/7 = 4 units each.
// Y drops by 0.5 per step: top = 3.0, bottom = 0.0.
static const int   N_STEPS   = 7;
static const float STEP_W    = 4.0f;    // each step width in X
static const float STEP_DROP = 0.5f;    // Y decrease per step
static const float TOP_Y     = 3.0f;   // surface Y of the topmost step

// Riser (vertical concrete face) dimensions
static const float RISER_THK = 0.30f;   // thickness of the riser face in X

// Colours
static const float CONC_R = 0.72f, CONC_G = 0.68f, CONC_B = 0.60f; // concrete riser
static const float EARTH_R = 0.28f, EARTH_G = 0.20f, EARTH_B = 0.10f;
static const float KERB_R  = 0.86f, KERB_G  = 0.86f, KERB_B  = 0.86f;

// Safe inner margin
static const float MARGIN = 1.2f;
static const float INNER_Z = HALF_D - MARGIN;   // 6.3

// Helpers

void Hole8::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole8::addFlagAt(float x, float baseY, float z)
{
    Mesh* pole = new Mesh(Mesh::makeCylinder(0.10f, 4.5f, 8,
                                              0.85f, 0.85f, 0.85f, 1.0f));
    pole->translate(x, baseY + 2.25f, z);
    pole->setup(); addMesh(pole);

    // Red pennant
    Mesh* pennant = new Mesh(Mesh::makeTriangularPrism(1.6f, 0.85f, 0.12f,
                                                        0.9f, 0.08f, 0.08f, 1.0f));
    pennant->translate(x + 0.8f, baseY + 4.4f, z);
    pennant->setup(); addMesh(pennant);
}

void Hole8::addFlowerAt(float x, float baseY, float z,
                         float pr, float pg, float pb)
{
    Mesh* stem = new Mesh(Mesh::makeCylinder(0.055f, 0.55f, 6,
                                              0.2f, 0.55f, 0.1f, 1.0f));
    stem->translate(x, baseY + 0.275f, z);
    stem->setup(); addMesh(stem);

    Mesh* petal = new Mesh(Mesh::makeCone(0.32f, 0.48f, 8, pr, pg, pb, 1.0f));
    petal->translate(x, baseY + 0.70f, z);
    petal->setup(); addMesh(petal);

    Mesh* centre = new Mesh(Mesh::makeCylinder(0.11f, 0.12f, 8,
                                                1.0f, 0.88f, 0.1f, 1.0f));
    centre->translate(x, baseY + 0.98f, z);
    centre->setup(); addMesh(centre);
}

void Hole8::addTree(float x, float baseY, float z, float scale)
{
    float s = scale;
    // Trunk : identical proportions to Hole5
    Mesh* trunk = new Mesh(Mesh::makeCylinder(0.32f * s, 4.0f * s, 8,
                                               0.44f, 0.29f, 0.11f, 1.0f));
    trunk->translate(x, baseY + 2.0f * s, z);
    trunk->setup(); addMesh(trunk);

    // Three stacked cones : identical to Hole5
    Mesh* c1 = new Mesh(Mesh::makeCone(2.4f * s, 3.2f * s, 10,
                                        0.10f, 0.44f, 0.14f, 1.0f));
    c1->translate(x, baseY + 5.0f * s, z);
    c1->setup(); addMesh(c1);

    Mesh* c2 = new Mesh(Mesh::makeCone(1.5f * s, 2.2f * s, 10,
                                        0.12f, 0.50f, 0.18f, 1.0f));
    c2->translate(x, baseY + 7.0f * s, z);
    c2->setup(); addMesh(c2);

    Mesh* c3 = new Mesh(Mesh::makeCone(0.9f * s, 1.5f * s, 10,
                                        0.14f, 0.55f, 0.20f, 1.0f));
    c3->translate(x, baseY + 8.8f * s, z);
    c3->setup(); addMesh(c3);
}


Hole8::Hole8() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    buildBorderWalls();
    buildSteps();
    buildRisers();
    buildRocks();
    buildFlag();
    buildTrees();
    buildFlowers();
}

Hole8::~Hole8()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

// Step geometry helpers
// stepX0(i) : left X boundary of step i
// stepSurfY(i): surface Y of step i

static float stepX0(int i)     { return -HALF_W + i * STEP_W; }
static float stepX1(int i)     { return -HALF_W + (i + 1) * STEP_W; }
static float stepCX(int i)     { return -HALF_W + (i + 0.5f) * STEP_W; }
static float stepSurfY(int i)  { return TOP_Y - i * STEP_DROP; }

// Green colour — linearly interpolates from bright (top) to darker (bottom)
static void stepColour(int i, float& r, float& g, float& b)
{
    float t = (float)i / (float)(N_STEPS - 1);   // 0 at top, 1 at bottom
    r = 0.13f - t * 0.04f;
    g = 0.58f - t * 0.16f;
    b = 0.14f - t * 0.03f;
}

void Hole8::buildBorderWalls()
{
    float wallXNeg = -HALF_W - BT * 0.5f;
    float wallXPos =  HALF_W + BT * 0.5f;
    float wallZNeg = -HALF_D - BT * 0.5f;
    float wallZPos =  HALF_D + BT * 0.5f;

    float tallH = TOP_Y + BH;   // side walls tall enough to clear top step

    // +Z wall
    Mesh* bZPos = new Mesh(Mesh::makeCuboid(HOLE_W, tallH, BT,
                                             KERB_R, KERB_G, KERB_B, 1.0f));
    bZPos->translate(0.0f, tallH * 0.5f, wallZPos);
    bZPos->setup(); addMesh(bZPos);

    // -Z wall
    Mesh* bZNeg = new Mesh(Mesh::makeCuboid(HOLE_W, tallH, BT,
                                             KERB_R, KERB_G, KERB_B, 1.0f));
    bZNeg->translate(0.0f, tallH * 0.5f, wallZNeg);
    bZNeg->setup(); addMesh(bZNeg);

    // -X wall (behind tee, full top-step height)
    float backH = TOP_Y + BH;
    Mesh* bXNeg = new Mesh(Mesh::makeCuboid(BT, backH, HOLE_D + BT,
                                             KERB_R, KERB_G, KERB_B, 1.0f));
    bXNeg->translate(wallXNeg, backH * 0.5f, 0.0f);
    bXNeg->setup(); addMesh(bXNeg);

    // +X wall (behind cup, just the kerb height)
    Mesh* bXPos = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT,
                                             KERB_R, KERB_G, KERB_B, 1.0f));
    bXPos->translate(wallXPos, BH * 0.5f, 0.0f);
    bXPos->setup(); addMesh(bXPos);
}

void Hole8::buildSteps()
{
    for (int i = 0; i < N_STEPS; i++)
    {
        float cx    = stepCX(i);
        float surfY = stepSurfY(i);
        float r, g, b;
        stepColour(i, r, g, b);

        Mesh* slab = new Mesh(Mesh::makeCuboid(STEP_W, FH, HOLE_D, r, g, b, 1.0f));
        slab->translate(cx, surfY + FH * 0.5f, 0.0f);
        slab->setup(); addMesh(slab);

        float fillH = surfY + 0.5f;
        if (fillH > 0.01f)
        {
            Mesh* fill = new Mesh(Mesh::makeCuboid(STEP_W, fillH, HOLE_D,
                                                    EARTH_R, EARTH_G, EARTH_B, 1.0f));
            fill->translate(cx, -0.5f + fillH * 0.5f, 0.0f);
            fill->setup(); addMesh(fill);
        }
    }
}

void Hole8::buildRisers()
{
    for (int i = 0; i < N_STEPS - 1; i++)
    {
        float upperY = stepSurfY(i);       // top of the riser
        float lowerY = stepSurfY(i + 1);   // bottom of the riser
        float riserH = upperY - lowerY;    // = STEP_DROP = 0.5

        float riserCX = stepX1(i);                // X boundary between steps
        float riserCY = lowerY + riserH * 0.5f;

        // Main concrete riser face
        Mesh* riser = new Mesh(Mesh::makeCuboid(RISER_THK, riserH, HOLE_D,
                                                 CONC_R, CONC_G, CONC_B, 1.0f));
        riser->translate(riserCX, riserCY, 0.0f);
        riser->setup(); addMesh(riser);

        // Thin concrete lip on the upper edge
        const float lipW = 0.22f;
        const float lipH = 0.10f;
        Mesh* lip = new Mesh(Mesh::makeCuboid(lipW, lipH, HOLE_D,
                                               CONC_R * 0.88f, CONC_G * 0.86f,
                                               CONC_B * 0.80f, 1.0f));
        lip->translate(riserCX - lipW * 0.5f, upperY + lipH * 0.5f, 0.0f);
        lip->setup(); addMesh(lip);
    }
}

void Hole8::buildRocks()
{
    // Two X offsets within each step (near left and right edge)
    const float OFF0 = 0.8f;   // distance from left  edge of step
    const float OFF1 = 3.2f;   // distance from left  edge (near right edge)

    for (int i = 0; i < N_STEPS; i++)
    {
        float x0    = stepX0(i);
        float surfY = stepSurfY(i);

        float xA = x0 + OFF0;
        float xB = x0 + OFF1;

        // -Z side (two rocks)
        for (float rx : { xA, xB })
        {
            float s = 0.55f + (i % 3) * 0.10f;   // gentle size variation
            Mesh* rock = new Mesh(Mesh::makeRock(s));
            rock->translate(rx, surfY + s * 0.9f, -(INNER_Z - 0.3f));
            rock->setup(); addMesh(rock);
        }

        // +Z side (two rocks)
        for (float rx : { xA, xB })
        {
            float s = 0.50f + (i % 2) * 0.12f;
            Mesh* rock = new Mesh(Mesh::makeRock(s));
            rock->translate(rx, surfY + s * 0.9f,  (INNER_Z - 0.3f));
            rock->setup(); addMesh(rock);
        }
    }
}

void Hole8::buildFlag()
{
    int   last   = N_STEPS - 1;
    float flagX  = stepX0(last) + STEP_W * 0.65f;   // 65% along the last step
    float flagY  = stepSurfY(last);                  // 0.0
    float flagZ  = 0.0f;

    Mesh* cup = new Mesh(Mesh::makeCylinder(0.48f, 0.28f, 12,
                                             0.10f, 0.10f, 0.10f, 1.0f));
    cup->translate(flagX, flagY + FH + 0.04f, flagZ);
    cup->setup(); addMesh(cup);

    addFlagAt(flagX, flagY + FH, flagZ);
}

void Hole8::buildTrees()
{
    struct TreeDef { float x, baseY, z, scale; };

    // One tree per step per side, centred on each step's X midpoint.
    // Alternating scale gives a natural look.
    float scales[N_STEPS] = { 0.85f, 0.78f, 0.82f, 0.75f, 0.80f, 0.76f, 0.83f };

    TreeDef trees[N_STEPS * 2];
    for (int i = 0; i < N_STEPS; i++)
    {
        float cx    = stepCX(i);
        float surfY = stepSurfY(i) + FH;
        float s     = scales[i];

        // -Z border tree
        trees[i * 2 + 0] = { cx, surfY, -5.2f, s };
        // +Z border tree
        trees[i * 2 + 1] = { cx, surfY,  5.0f, s * 0.93f };
    }

    for (auto& td : trees)
        addTree(td.x, td.baseY, td.z, td.scale);
}

void Hole8::buildFlowers()
{
    // Colour palette cycling across steps, matching Hole5 style
    float FR[] = { 0.95f, 1.00f, 0.85f, 0.90f, 1.00f, 0.80f, 0.95f };
    float FG[] = { 0.30f, 0.75f, 0.20f, 0.85f, 0.40f, 0.25f, 0.88f };
    float FB[] = { 0.60f, 0.10f, 0.80f, 0.10f, 0.10f, 0.70f, 0.10f };

    for (int i = 0; i < N_STEPS; i++)
    {
        float cx    = stepCX(i);
        float baseY = stepSurfY(i) + FH;
        int   ci    = i % 7;
        int   ci2   = (i + 3) % 7;

        // -Z inner edge flower
        addFlowerAt(cx, baseY, -4.5f, FR[ci],  FG[ci],  FB[ci]);
        // +Z inner edge flower
        addFlowerAt(cx, baseY,  4.2f, FR[ci2], FG[ci2], FB[ci2]);
    }

    // Extra flowers near the flag on the bottom step
    float lastY = stepSurfY(N_STEPS - 1) + FH;
    addFlowerAt(stepX0(N_STEPS - 1) + 2.8f, lastY,  2.5f, 0.95f, 0.88f, 0.10f);
    addFlowerAt(stepX0(N_STEPS - 1) + 1.5f, lastY, -2.0f, 1.00f, 0.35f, 0.55f);
}

void Hole8::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole8::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}

void Hole8::draw(GLuint shader, bool wireframe,
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