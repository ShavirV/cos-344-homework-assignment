#include "Hole5.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>


static const float HOLE_W = 28.0f;   // total X extent
static const float HOLE_D = 15.0f;   // total Z extent
static const float HALF_W = HOLE_W * 0.5f;   // 14
static const float HALF_D = HOLE_D * 0.5f;   // 7.5

// Slab / kerb geometry
static const float FH = 0.2f;   // fairway slab height
static const float BT = 0.5f;   // border thickness
static const float BH = 0.4f;   // border kerb height

// Elevation
static const float RISE = 2.5f;
// Sub-section X extents
static const float UPPER_END = 0.0f;     
static const float RAMP_LEN = 5.0f; // length of ramp in X
static const float RAMP_END = UPPER_END + RAMP_LEN; // 5.0
static const float LOWER_W = HALF_W - RAMP_END; // 9.0
static const float LOWER_CX = RAMP_END + LOWER_W * 0.5f; // centre X of lower pad

// Colours
static const float GR = 0.13f, GG = 0.58f, GB = 0.14f; // upper green
static const float LR = 0.10f, LG = 0.48f, LB = 0.12f;  // lower green (darker)
static const float EARTH_R = 0.28f, EARTH_G = 0.20f, EARTH_B = 0.10f;
static const float CONC_R = 0.86f, CONC_G = 0.86f, CONC_B = 0.86f;
static const float WTR_R = 0.15f, WTR_G = 0.45f, WTR_B = 0.88f;

// Safe inner margin: keep decorations this far from the border walls
static const float MARGIN = 1.2f;
static const float INNER_X = HALF_W - MARGIN; // 12.8
static const float INNER_Z = HALF_D - MARGIN; // 6.3


void Hole5::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole5::addFlagAt(float x, float baseY, float z)
{
    Mesh* pole = new Mesh(Mesh::makeCylinder(0.10f, 4.5f, 8, 0.85f, 0.85f, 0.85f, 1.0f));
    pole->translate(x, baseY + 2.25f, z);
    pole->setup();
    addMesh(pole);

    Mesh* pennant = new Mesh(Mesh::makeTriangularPrism(1.6f, 0.85f, 0.12f, 0.9f, 0.08f, 0.08f, 1.0f));
    pennant->translate(x + 0.8f, baseY + 4.4f, z);
    pennant->setup();
    addMesh(pennant);
}

void Hole5::addFlowerAt(float x, float baseY, float z, float pr, float pg, float pb)
{
    Mesh* stem = new Mesh(Mesh::makeCylinder(0.055f, 0.55f, 6, 0.2f, 0.55f, 0.1f, 1.0f));
    stem->translate(x, baseY + 0.275f, z);
    stem->setup();
    addMesh(stem);

    Mesh* petal = new Mesh(Mesh::makeCone(0.32f, 0.48f, 8, pr, pg, pb, 1.0f));
    petal->translate(x, baseY + 0.70f, z);
    petal->setup();
    addMesh(petal);

    Mesh* centre = new Mesh(Mesh::makeCylinder(0.11f, 0.12f, 8, 1.0f, 0.88f, 0.1f, 1.0f));
    centre->translate(x, baseY + 0.98f, z);
    centre->setup();
    addMesh(centre);
}

void Hole5::addTree(float x, float baseY, float z, float scale)
{
    float s = scale;
    Mesh* trunk = new Mesh(Mesh::makeCylinder(0.32f * s, 4.0f * s, 8, 0.44f, 0.29f, 0.11f, 1.0f));
    trunk->translate(x, baseY + 2.0f * s, z);
    trunk->setup();
    addMesh(trunk);

    Mesh* c1 = new Mesh(Mesh::makeCone(2.4f * s, 3.2f * s, 10, 0.10f, 0.44f, 0.14f, 1.0f));
    c1->translate(x, baseY + 5.0f * s, z);
    c1->setup();
    addMesh(c1);

    Mesh* c2 = new Mesh(Mesh::makeCone(1.5f * s, 2.2f * s, 10, 0.12f, 0.50f, 0.18f, 1.0f));
    c2->translate(x, baseY + 7.0f * s, z);
    c2->setup();
    addMesh(c2);
}

Hole5::Hole5() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    buildFairways();
    buildWaterfall();
    buildStream();
    buildRocks();
    buildFlag();
    buildTrees();
    buildFlowers();
}

Hole5::~Hole5()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}


void Hole5::buildFairways()
{
    float upperCX = ((-HALF_W) + UPPER_END) * 0.5f; // centre X of upper fairway
    float upperW  = UPPER_END - (-HALF_W); // 14.0

    float wallXNeg = -HALF_W - BT * 0.5f; // -14.25
    float wallXPos = HALF_W + BT * 0.5f; //  14.25
    float wallZNeg = -HALF_D - BT * 0.5f; // -7.75
    float wallZPos = HALF_D + BT * 0.5f; //  7.75

    // +Z wall
    Mesh* bZPos = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CONC_R, CONC_G, CONC_B, 1.0f));
    bZPos->translate(0.0f, BH * 0.5f, wallZPos);
    bZPos->setup(); addMesh(bZPos);

    // -Z wall
    Mesh* bZNeg = new Mesh(Mesh::makeCuboid(HOLE_W, BH, BT, CONC_R, CONC_G, CONC_B, 1.0f));
    bZNeg->translate(0.0f, BH * 0.5f, wallZNeg);
    bZNeg->setup(); addMesh(bZNeg);

    // -X wall (behind waterfall)
    Mesh* bXNeg = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT, CONC_R, CONC_G, CONC_B, 1.0f));
    bXNeg->translate(wallXNeg, BH * 0.5f, 0.0f);
    bXNeg->setup(); addMesh(bXNeg);

    // +X wall (behind lower pad)
    Mesh* bXPos = new Mesh(Mesh::makeCuboid(BT, BH, HOLE_D + BT, CONC_R, CONC_G, CONC_B, 1.0f));
    bXPos->translate(wallXPos, BH * 0.5f, 0.0f);
    bXPos->setup(); addMesh(bXPos);

    Mesh* upper = new Mesh(Mesh::makeCuboid(upperW, FH, HOLE_D,
                                             GR, GG, GB, 1.0f));
    upper->translate(upperCX, RISE + FH * 0.5f, 0.0f);
    upper->setup(); addMesh(upper);

    // Earth support column under the upper fairway so the sides look solid
    Mesh* earthCol = new Mesh(Mesh::makeCuboid(upperW, RISE, HOLE_D,
                                                EARTH_R, EARTH_G, EARTH_B, 1.0f));
    earthCol->translate(upperCX, RISE * 0.5f, 0.0f);
    earthCol->setup(); addMesh(earthCol);

    //ramp 
    {
        const int   N    = 20;
        const float step = RAMP_LEN / (float)N;  
        for (int i = 0; i < N; i++)
        {
            
            // Use the average (centre top) as the height for makeCuboid
            float tLeft  = RISE * (1.0f - (float)i       / (float)N);
            float tRight = RISE * (1.0f - (float)(i + 1) / (float)N);
            float sliceH = (tLeft + tRight) * 0.5f + FH;   // full height from Y=0

            float sliceCX = UPPER_END + (i + 0.5f) * step; // centre X of this slice

            // Slightly blend the green colour darker as we descend (visual cue)
            float t  = (float)i / (float)(N - 1);
            float sr = GR * 0.92f + (LR - GR * 0.92f) * t;
            float sg = GG * 0.92f + (LG - GG * 0.92f) * t;
            float sb = GB         + (LB - GB)           * t;

            Mesh* slice = new Mesh(Mesh::makeCuboid(step, sliceH, HOLE_D,
                                                    sr, sg, sb, 1.0f));
            // Bottom of slice is at Y=0, centre at Y = sliceH * 0.5
            slice->translate(sliceCX, sliceH * 0.5f, 0.0f);
            slice->setup(); addMesh(slice);
        }
    }

    Mesh* lower = new Mesh(Mesh::makeCuboid(LOWER_W, FH, HOLE_D,
                                             LR, LG, LB, 1.0f));
    lower->translate(LOWER_CX, FH * 0.5f, 0.0f);
    lower->setup(); addMesh(lower);
}


void Hole5::buildWaterfall()
{
    const float WF_OFFSET = -3.0f;   // shift waterfall further left (outside)
    // Tier widths (protrusion towards the course), measured from the back wall
    const float tProtrude[3] = { 2.5f, 4.5f, 7.0f };
    const float tHeight[3]   = { 2.0f, 4.0f, 6.5f };
    const float wfD = HOLE_D - 2.0f * MARGIN; // Z extent stays inside

    for (int i = 0; i < 3; i++)
    {
        float ledgeW = (i == 0) ? tProtrude[0]
                                : tProtrude[i] - tProtrude[i - 1];
        float ledgeCX = -HALF_W + WF_OFFSET + tProtrude[i] - ledgeW * 0.5f;

        // Concrete ledge
        Mesh* ledge = new Mesh(Mesh::makeCuboid(ledgeW, tHeight[i], wfD,
                                                 CONC_R * 0.82f, CONC_G * 0.80f,
                                                 CONC_B * 0.74f, 1.0f));
        ledge->translate(ledgeCX, tHeight[i] * 0.5f, 0.0f);
        ledge->setup(); addMesh(ledge);

        // Water face on the +X side of the tier
        float waterH = tHeight[i];
        float waterX = -HALF_W + WF_OFFSET + tProtrude[i] + 0.55f;
        float waterD = wfD - 0.4f;

        Mesh* waterFace = new Mesh(Mesh::makeCuboid(1.1f, waterH, waterD,
                                                     WTR_R, WTR_G, WTR_B, 0.88f));
        waterFace->translate(waterX, waterH * 0.5f, 0.0f);
        waterFace->setup(); addMesh(waterFace);

        // Horizontal water pool on top of each ledge
        Mesh* topWater = new Mesh(Mesh::makeCuboid(ledgeW - 0.1f, 0.16f,
                                                    waterD - 0.2f,
                                                    WTR_R, WTR_G, WTR_B, 0.85f));
        topWater->translate(ledgeCX, tHeight[i] + 0.08f, 0.0f);
        topWater->setup(); addMesh(topWater);
    }

    // Base pool (optional, placed just behind the wall)
    float poolFrontX  = -HALF_W + WF_OFFSET + tProtrude[0] + 1.1f;
    float poolBackX   = -HALF_W - 0.5f;
    float poolW       = poolBackX - poolFrontX;
    if (poolW > 0.2f)
    {
        Mesh* basePool = new Mesh(Mesh::makeCuboid(poolW, 0.20f, wfD,
                                                    WTR_R, WTR_G, WTR_B, 0.90f));
        basePool->translate(poolFrontX + poolW * 0.5f, 0.10f, 0.0f);
        basePool->setup(); addMesh(basePool);
    }
}


void Hole5::buildStream()
{
    const float streamW  = 2.0f;
    const float upperW   = UPPER_END - (-HALF_W);
    const float streamCX = (-HALF_W + UPPER_END) * 0.5f;
    const float streamCZ = INNER_Z - streamW * 0.5f;

    Mesh* stream = new Mesh(Mesh::makeCuboid(upperW - 1.0f, 0.14f, streamW,
                                              WTR_R, WTR_G, WTR_B, 0.88f));
    stream->translate(streamCX, RISE + 0.07f, streamCZ);
    stream->setup(); addMesh(stream);

    Mesh* bank = new Mesh(Mesh::makeCuboid(upperW - 1.0f, 0.18f, 1.2f,
                                            0.22f, 0.35f, 0.14f, 1.0f));
    bank->translate(streamCX, RISE + 0.09f, streamCZ + streamW * 0.5f + 0.6f);
    bank->setup(); addMesh(bank);
}


void Hole5::buildRocks()
{
    struct RockDef { float x, surfY, z, s; };

    RockDef rocks[] = {
        // -Z side of upper fairway
        { -11.0f, RISE, -(INNER_Z - 0.6f), 0.80f },
        {  -7.0f, RISE, -(INNER_Z - 0.5f), 0.95f },
        {  -3.0f, RISE, -(INNER_Z - 0.7f), 0.75f },

        // +Z side of upper fairway (beside stream)
        { -10.0f, RISE,  (INNER_Z - 1.8f), 0.70f },
        {  -5.0f, RISE,  (INNER_Z - 1.8f), 0.85f },
        {   -1.0f, RISE,  (INNER_Z - 1.6f), 0.75f },

        // Lower pad flanking the cup
        {  LOWER_CX + 2.0f, 0.0f, -(INNER_Z - 0.8f), 0.72f },
        {  LOWER_CX + 2.0f, 0.0f,  (INNER_Z - 0.8f), 0.72f },
        {  LOWER_CX - 2.5f, 0.0f, -(INNER_Z - 0.6f), 0.68f },
    };

    for (auto& rd : rocks)
    {
        Mesh* rock = new Mesh(Mesh::makeRock(rd.s));
        rock->translate(rd.x, rd.surfY + rd.s * 1.0f, rd.z);
        rock->setup();
        addMesh(rock);
    }
}

void Hole5::buildFlag()
{
    float flagX = LOWER_CX;
    float flagZ = 0.5f;

    Mesh* cup = new Mesh(Mesh::makeCylinder(0.48f, 0.28f, 12,
                                             0.10f, 0.10f, 0.10f, 1.0f));
    cup->translate(flagX, FH + 0.04f, flagZ);
    cup->setup(); addMesh(cup);

    addFlagAt(flagX, FH, flagZ);
}


void Hole5::buildTrees()
{
    struct TreeDef { float x, baseY, z, scale; };
    TreeDef trees[] = {
        // Upper fairway — along -Z border
        { -12.0f, RISE + FH, -5.5f, 0.85f },
        {  -7.5f, RISE + FH, -5.2f, 0.75f },
        {  -3.0f, RISE + FH, -5.5f, 0.90f },

        // Upper fairway — along +Z border (near stream)
        { -11.0f, RISE + FH,  4.5f, 0.80f },
        {  -5.5f, RISE + FH,  4.8f, 0.88f },

        // Lower pad — corners
        {  LOWER_CX + 3.0f, FH, -5.2f, 0.78f },
        {  LOWER_CX + 3.0f, FH,  5.0f, 0.82f },
        {  LOWER_CX - 2.0f, FH, -5.5f, 0.72f },
    };

    for (auto& td : trees)
        addTree(td.x, td.baseY, td.z, td.scale);
}


void Hole5::buildFlowers()
{
    float uY = RISE + FH;
    float lY = FH;

    struct FlowerDef { float x, y, z; float r, g, b; };
    FlowerDef flowers[] = {
        // Upper fairway -Z inner edge
        { -12.5f, uY, -4.8f, 0.95f, 0.30f, 0.60f },
        {  -8.5f, uY, -5.0f, 1.00f, 0.75f, 0.10f },
        {  -4.5f, uY, -4.7f, 0.85f, 0.20f, 0.80f },

        // Upper fairway +Z inner edge
        { -11.0f, uY,  3.8f, 0.90f, 0.85f, 0.10f },
        {  -6.0f, uY,  4.2f, 0.80f, 0.25f, 0.70f },
        {  -1.5f, uY,  3.9f, 1.00f, 0.40f, 0.10f },

        // Near flag on lower pad
        { LOWER_CX + 1.8f, lY,  2.5f, 0.95f, 0.88f, 0.10f },
        { LOWER_CX - 1.5f, lY, -2.0f, 1.00f, 0.35f, 0.55f },
        { LOWER_CX + 2.5f, lY, -1.8f, 0.90f, 0.30f, 0.60f },
    };

    for (auto& fd : flowers)
        addFlowerAt(fd.x, fd.y, fd.z, fd.r, fd.g, fd.b);
}


void Hole5::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole5::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}

void Hole5::draw(GLuint shader, bool wireframe,
                 const glm::vec3& lightDir,
                 const glm::vec3& lightColor,
                 const glm::vec3& camPos)
{
    // Set the per-frame directional-light uniforms that dev.cpp does NOT push
    // program-wide (uLightDir and uViewPos). uLightColor and uAmbient are
    // already set by dev.cpp before this draw() is called, so we skip them.
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