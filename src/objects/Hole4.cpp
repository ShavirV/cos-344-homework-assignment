#include "Hole4.h"
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

#define C_GREEN      0.15f, 0.50f, 0.15f, 1.0f
#define C_CREAM      0.93f, 0.90f, 0.80f, 1.0f
#define C_DARKBROWN  0.32f, 0.20f, 0.12f, 1.0f
#define C_BROWN      0.38f, 0.24f, 0.12f, 1.0f
#define C_YELLOW     0.95f, 0.84f, 0.22f, 1.0f
#define C_RED        0.82f, 0.10f, 0.10f, 1.0f
#define C_METAL      0.68f, 0.68f, 0.72f, 1.0f
#define C_WHITE      0.96f, 0.96f, 0.96f, 1.0f
#define C_WOODDARK   0.38f, 0.26f, 0.14f, 1.0f
#define C_DARKGRASS  0.08f, 0.32f, 0.08f, 1.0f
#define C_SAND       0.93f, 0.85f, 0.60f, 1.0f
#define C_FLAGRED    0.90f, 0.10f, 0.10f, 1.0f
#define C_START      0.85f, 0.60f, 0.20f, 1.0f
#define C_BLACK      0.04f, 0.04f, 0.04f, 1.0f
#define C_STEMGREEN  0.20f, 0.55f, 0.20f, 1.0f
#define C_FLWPINK    0.95f, 0.45f, 0.65f, 1.0f
#define C_FLWYELLOW  0.98f, 0.88f, 0.20f, 1.0f

static constexpr float S = 2.0f;

static constexpr float SPINE_OFF_X = 0.25f * S;
static constexpr float BLADE_OFF_Y = 0.90f * S;
static constexpr float SPINE_HALF  = (0.90f * S) * 0.5f;
static constexpr float ARM_LEN     = BLADE_OFF_Y - SPINE_HALF;
static constexpr float ARM_CY      = ARM_LEN * 0.5f;
static constexpr float SLAT_START  = BLADE_OFF_Y - SPINE_HALF + 0.10f;
static constexpr float SLAT_STEP   = (SPINE_HALF * 2.0f - 0.10f) / 7.0f;

static Mesh* place(Mesh&& src, float x, float y, float z,
                   Mesh** pool, int& cnt, int maxCnt)
{
    Mesh* m = new Mesh(std::move(src));
    m->translate(x, y, z);
    m->setup();
    if (cnt < maxCnt) pool[cnt++] = m;
    return m;
}

Hole4::Hole4()
    : meshCount(0),
      pivotX(0.f), pivotY(0.f), pivotZ(0.f),
      bladeAngle(0.f), lastDrawTime(-1.0)
{
    for (int i = 0; i < MAX_MESHES; ++i) meshes[i] = nullptr;
    for (int i = 0; i < 4; ++i) {
        bladeArm[i] = bladeSpineL[i] = bladeSpineR[i] = nullptr;
        for (int j = 0; j < 8; ++j) bladeSlat[i][j] = nullptr;
    }

    const float SY = 0.0f;
    const float groundHeight = 0.2f;

    // Base ground platform (28 x 15 as requested)
    place(Mesh::makeCuboid(28.0f, groundHeight, 15.0f, C_DARKGRASS),
        0.0f, SY + groundHeight * 0.5f, 0.0f,
        meshes, meshCount, MAX_MESHES);

    const float pathHeight = 0.3f;
    const float pathY = SY + groundHeight + pathHeight * 0.5f;

    // Bottom horizontal bar (fits within X: -12 to +12, Z: -6)
    place(Mesh::makeCuboid(24.0f, pathHeight, 4.0f, C_GREEN),
        0.0f, pathY, -5.0f,
        meshes, meshCount, MAX_MESHES);

    // Right vertical bar (fits within X: +10, Z: -5 to +5)
    place(Mesh::makeCuboid(4.0f, pathHeight, 12.0f, C_GREEN),
        10.0f, pathY, 0.0f,
        meshes, meshCount, MAX_MESHES);

    // Top horizontal bar (fits within X: -2 to +12, Z: +5)
    place(Mesh::makeCuboid(14.0f, pathHeight, 4.0f, C_GREEN),
        5.0f, pathY, 5.0f,
        meshes, meshCount, MAX_MESHES);

    const float WX = 10.f;
    const float WZ = 0.f;
    const float WY = pathY - 0.15f;

    const float drumHalfH   = 0.36f * S;
    const float drumTopY    = WY + drumHalfH * 2.f;
    const float galleryY    = drumTopY + 0.10f;
    const float shingleY    = galleryY + 0.28f;
    const float coneBaseY   = shingleY + 0.18f;
    const float coneH       = 1.80f * S;
    const float coneCentreY = coneBaseY + coneH * 0.5f;
    const float coneTopY    = coneBaseY + coneH;
    const float hubY        = coneTopY + 0.28f;

    const float slabW    = 0.20f * S;
    const float slabOffX = 0.40f * S;
    const float slabD    = 0.65f * S;
    const float slabH    = shingleY - WY;

    // Left slab
    place(Mesh::makeCuboid(slabW, slabH, slabD * 2.f, C_CREAM),
        WX - slabOffX, WY + slabH * 0.5f, WZ, meshes, meshCount, MAX_MESHES);
    // Right slab
    place(Mesh::makeCuboid(slabW, slabH, slabD * 2.f, C_CREAM),
        WX + slabOffX, WY + slabH * 0.5f, WZ, meshes, meshCount, MAX_MESHES);
    // Base rim cylinder
    place(Mesh::makeCylinder(slabD, 0.06f * S, 24, C_DARKBROWN),
        WX, WY + 0.06f, WZ, meshes, meshCount, MAX_MESHES);

    // Gallery ring + rail + 12 posts
    const float galR = 0.72f * S;
    place(Mesh::makeCylinder(galR, 0.10f * S, 24, C_RED),
        WX, galleryY + 0.05f * S, WZ, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCylinder(galR + 0.04f, 0.05f * S, 24, C_RED),
        WX, galleryY + 0.24f * S, WZ, meshes, meshCount, MAX_MESHES);
    for (int i = 0; i < 12; ++i) {
        float ang = i * 2.f * (float)M_PI / 12.f;
        place(Mesh::makeCuboid(0.04f * S, 0.22f * S, 0.04f * S, C_RED),
            WX + galR * cosf(ang), galleryY + 0.14f * S,
            WZ + galR * sinf(ang), meshes, meshCount, MAX_MESHES);
    }

    // Shingle band + cone rim
    place(Mesh::makeCylinder(0.70f * S, 0.18f * S, 24, C_DARKBROWN),
        WX, shingleY, WZ, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCylinder(0.68f * S, 0.05f * S, 24, C_DARKBROWN),
        WX, shingleY + 0.11f * S, WZ, meshes, meshCount, MAX_MESHES);

    // Main cone
    place(Mesh::makeCone(0.65f * S, coneH, 24, C_YELLOW),
        WX, coneCentreY, WZ, meshes, meshCount, MAX_MESHES);

    // Hub box
    place(Mesh::makeCuboid(0.28f * S, 0.28f * S, 0.28f * S, C_DARKBROWN),
        WX, hubY, WZ, meshes, meshCount, MAX_MESHES);

    // Axle
    const float axleLen    = 0.45f * S;
    const float axleTipZ   = WZ + axleLen * 0.5f + 0.14f * S;
    {
        Mesh axle = Mesh::makeCylinder(0.06f * S, axleLen, 12, C_METAL);
        axle.rotate(90.f, 1.f, 0.f, 0.f);
        axle.translate(WX, hubY, WZ + axleLen * 0.5f);
        axle.setup();
        addMesh(new Mesh(std::move(axle)));
    }
    // Rotor hub disc
    {
        Mesh rh = Mesh::makeCylinder(0.14f * S, 0.12f * S, 12, C_METAL);
        rh.rotate(90.f, 1.f, 0.f, 0.f);
        rh.translate(WX, hubY, axleTipZ);
        rh.setup();
        addMesh(new Mesh(std::move(rh)));
    }
 
    // Pivot
    pivotX = WX;
    pivotY = hubY;
    pivotZ = axleTipZ + 0.07f * S;
    


    // Four rotor arms
    for (int i = 0; i < 4; ++i) buildBlade(i);

    // Decorative window cross
    {
        float wz = WZ + slabD + 0.02f;
        place(Mesh::makeCuboid(0.14f * S, 0.03f * S, 0.03f * S, C_WOODDARK),
            WX + slabOffX, WY + drumHalfH, wz, meshes, meshCount, MAX_MESHES);
        place(Mesh::makeCuboid(0.03f * S, 0.14f * S, 0.03f * S, C_WOODDARK),
            WX + slabOffX, WY + drumHalfH, wz, meshes, meshCount, MAX_MESHES);
    }

    const float groundY = SY + groundHeight;

    // Trees along the path edges - ONLY ON GREEN
    // Bottom bar trees
    place(Mesh::makeCylinder(0.16f, 0.80f, 8, C_BROWN),
          -8.0f, pathY + 0.40f, -4.0f, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCone(0.56f, 1.30f, 10, C_DARKGRASS),
          -8.0f, pathY + 0.90f, -4.0f, meshes, meshCount, MAX_MESHES);
    
    place(Mesh::makeCylinder(0.16f, 0.80f, 8, C_BROWN),
          5.0f, pathY + 0.40f, -4.0f, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCone(0.56f, 1.30f, 10, C_DARKGRASS),
          5.0f, pathY + 0.90f, -4.0f, meshes, meshCount, MAX_MESHES);
    
    // Right bar trees
    place(Mesh::makeCylinder(0.16f, 0.80f, 8, C_BROWN),
          12.0f, pathY + 0.40f, 2.0f, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCone(0.56f, 1.30f, 10, C_DARKGRASS),
          12.0f, pathY + 0.90f, 2.0f, meshes, meshCount, MAX_MESHES);
    
    place(Mesh::makeCylinder(0.16f, 0.80f, 8, C_BROWN),
          12.0f, pathY + 0.40f, 6.0f, meshes, meshCount, MAX_MESHES);
    place(Mesh::makeCone(0.56f, 1.30f, 10, C_DARKGRASS),
          12.0f, pathY + 0.90f, 6.0f, meshes, meshCount, MAX_MESHES);
    
    // Cup 
    place(Mesh::makeCylinder(0.36f, 0.12f, 16, C_BLACK),
        -11.0f, pathY + 0.1f, -5.5f, meshes, meshCount, MAX_MESHES);

    // Flag pole ON GREEN at bottom left
    place(Mesh::makeCylinder(0.05f, 1.60f, 8, C_METAL),
        -11.0f, pathY + 0.1f, -5.5f, meshes, meshCount, MAX_MESHES);

    // Tee mat - BRIGHT YELLOW with white border ON GREEN PATH (top left)
    Mesh teeMat = Mesh::makeCuboid(2.5f, 0.05f, 2.5f, 1.0f, 0.8f, 0.0f, 1.0f);
    teeMat.translate(1.0f, pathY + 0.2f, 5.5f);
    teeMat.setup();
    addMesh(new Mesh(std::move(teeMat)));

    // White border
    Mesh teeBorder = Mesh::makeCuboid(2.65f, 0.01f, 2.65f, 1.0f, 1.0f, 1.0f, 1.0f);
    teeBorder.translate(1.0f, pathY + 0.2f, 5.5f);
    teeBorder.setup();
    addMesh(new Mesh(std::move(teeBorder)));
    updateBladeTransforms();
}

void Hole4::buildBlade(int arm)
{
    // Arm bar: from pivot (Y=0) up to base of spines
    {
        Mesh* ab = new Mesh(Mesh::makeCuboid(0.10f * S, ARM_LEN, 0.10f * S, C_RED));
        ab->setup();
        bladeArm[arm] = ab;
        addMesh(ab);
    }
    // Left spine
    {
        Mesh* sp = new Mesh(Mesh::makeCuboid(0.10f * S, 0.90f * S, 0.10f * S, C_RED));
        sp->setup();
        bladeSpineL[arm] = sp;
        addMesh(sp);
    }
    // Right spine
    {
        Mesh* sp = new Mesh(Mesh::makeCuboid(0.10f * S, 0.90f * S, 0.10f * S, C_RED));
        sp->setup();
        bladeSpineR[arm] = sp;
        addMesh(sp);
    }
    // 8 slats
    for (int j = 0; j < 8; ++j) {
        Mesh* sl = new Mesh(Mesh::makeCuboid(
            SPINE_OFF_X * 2.f, 0.055f * S, 0.04f * S, C_WHITE));
        sl->setup();
        bladeSlat[arm][j] = sl;
        addMesh(sl);
    }
}

void Hole4::updateBladeTransforms()
{
    glm::mat4 toPivot = glm::translate(glm::mat4(1.f),
                                       glm::vec3(pivotX, pivotY, pivotZ));

    for (int arm = 0; arm < 4; ++arm) {
        if (!bladeSpineL[arm]) continue;

        float ang  = bladeAngle + arm * (float)M_PI * 0.5f;
        glm::mat4 spin = glm::rotate(glm::mat4(1.f), ang,
                                     glm::vec3(0.f, 0.f, 1.f));

        if (bladeArm[arm])
            bladeArm[arm]->modelMatrix = toPivot * spin *
                glm::translate(glm::mat4(1.f), glm::vec3(0.f, ARM_CY, 0.f));

        bladeSpineL[arm]->modelMatrix = toPivot * spin *
            glm::translate(glm::mat4(1.f), glm::vec3(-SPINE_OFF_X, BLADE_OFF_Y, 0.f));

        bladeSpineR[arm]->modelMatrix = toPivot * spin *
            glm::translate(glm::mat4(1.f), glm::vec3(SPINE_OFF_X, BLADE_OFF_Y, 0.f));

        for (int j = 0; j < 8; ++j) {
            if (!bladeSlat[arm][j]) continue;
            float sY = SLAT_START + j * SLAT_STEP;
            bladeSlat[arm][j]->modelMatrix = toPivot * spin *
                glm::translate(glm::mat4(1.f), glm::vec3(0.f, sY, 0.08f * S));
        }
    }
}

Hole4::~Hole4()
{
    for (int i = 0; i < meshCount; ++i) delete meshes[i];
}

Mesh* Hole4::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES) meshes[meshCount++] = m;
    return m;
}

void Hole4::draw(GLuint shaderProgram, bool wireframe,
                 const glm::vec3& lightDir,
                 const glm::vec3& lightColor,
                 const glm::vec3& camPos)
{
    double now = glfwGetTime();
    if (lastDrawTime < 0.0) lastDrawTime = now;
    float dt = (float)(now - lastDrawTime);
    lastDrawTime = now;
    if (dt > 0.1f) dt = 0.1f;

    bladeAngle += BLADE_SPEED * dt;
    if (bladeAngle > 2.f * (float)M_PI)
        bladeAngle -= 2.f * (float)M_PI;

    updateBladeTransforms();

    glUseProgram(shaderProgram);
    GLint locLD = glGetUniformLocation(shaderProgram, "uLightDir");
    GLint locVP = glGetUniformLocation(shaderProgram, "uViewPos");
    GLint locLC = glGetUniformLocation(shaderProgram, "uLightColor");
    if (locLD != -1) glUniform3fv(locLD, 1, glm::value_ptr(glm::normalize(lightDir)));
    if (locVP != -1) glUniform3fv(locVP, 1, glm::value_ptr(camPos));
    if (locLC != -1) glUniform3fv(locLC, 1, glm::value_ptr(lightColor));

    for (int i = 0; i < meshCount; ++i)
        if (meshes[i]) meshes[i]->draw(shaderProgram, wireframe);
}

void Hole4::drawDepth(GLuint depthShader)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->drawDepth(depthShader);
}

void Hole4::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; ++i)
        if (meshes[i]) meshes[i]->translate(x, y, z);
    pivotX += x;  pivotY += y;  pivotZ += z;
}

void Hole4::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; ++i)
        if (meshes[i]) meshes[i]->rotate(angle, ax, ay, az);
}