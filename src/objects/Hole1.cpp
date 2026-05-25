#include "Hole1.h"
#include <cmath>
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

// ─── Colours ──────────────────────────────────────────────────────────────────
#define C_GREEN         0.15f, 0.50f, 0.15f, 1.0f
#define C_DARKGRASS     0.08f, 0.32f, 0.08f, 1.0f
#define C_SAND          0.93f, 0.85f, 0.60f, 1.0f
#define C_WATER         0.20f, 0.60f, 0.85f, 0.9f
#define C_BROWN         0.55f, 0.35f, 0.20f, 1.0f
#define C_GREY          0.60f, 0.60f, 0.65f, 1.0f
#define C_FLOWER_RED    0.95f, 0.30f, 0.30f, 1.0f
#define C_FLOWER_YELLOW 0.95f, 0.85f, 0.20f, 1.0f

Hole1::Hole1()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ─── BASE GROUND (28 x 15) ───────────────────────────────────────────────
    const float SY = 0.0f;
    const float groundHeight = 0.2f;

    Mesh* ground = new Mesh(Mesh::makeCuboid(28.0f, groundHeight, 15.0f, C_DARKGRASS));
    ground->translate(0.0f, SY + groundHeight * 0.5f, 0.0f);
    ground->setup();
    addMesh(ground);

    // ─── L-SHAPED FAIRWAY ────────────────────────────────────────────────────
    const float FH = 0.2f;
    const float FW = 5.0f;
    const float pathY = SY + groundHeight + FH * 0.5f;
    const float slopeH = 0.5f;

    Mesh* verticalFairway = new Mesh(Mesh::makeHill(
        FW, 4.0f, FW + 1.0f, 6.0f, slopeH, C_GREEN
    ));
    verticalFairway->translate(-5.0f, pathY + slopeH * 0.5f, -1.0f);
    verticalFairway->setup();
    addMesh(verticalFairway);

    Mesh* horizontalFairway = new Mesh(Mesh::makeCuboid(18.0f, FH, FW, C_GREEN));
    horizontalFairway->translate(1.0f, pathY, 4.0f);
    horizontalFairway->setup();
    addMesh(horizontalFairway);

    // ─── SAND TRAP ────────────────────────────────────────────────────────────
    Mesh* sandTrap = new Mesh(Mesh::makeCylinder(1.5f, 0.1f, 20, C_SAND));
    sandTrap->translate(6.0f, pathY - 0.02f, 2.0f);
    sandTrap->setup();
    addMesh(sandTrap);

    // ─── WATER HAZARD ─────────────────────────────────────────────────────────
    Mesh* water = new Mesh(Mesh::makeCylinder(1.2f, 0.05f, 24, C_WATER));
    water->translate(-2.0f, pathY - 0.03f, -2.0f);
    water->setup();
    addMesh(water);

    Mesh* waterEdge = new Mesh(Mesh::makeCylinder(1.35f, 0.04f, 24, 0.85f, 0.70f, 0.50f, 1.0f));
    waterEdge->translate(-2.0f, pathY - 0.04f, -2.0f);
    waterEdge->setup();
    addMesh(waterEdge);

    // ─── TEE MAT ──────────────────────────────────────────────────────────────
    Mesh* teeMat = new Mesh(Mesh::makeCuboid(1.5f, 0.08f, 1.5f, 1.0f, 0.8f, 0.0f, 1.0f));
    teeMat->translate(-5.0f, pathY + slopeH + 0.04f, -1.0f);
    teeMat->setup();
    addMesh(teeMat);

    Mesh* teeBorder = new Mesh(Mesh::makeCuboid(1.6f, 0.02f, 1.6f, 1.0f, 1.0f, 1.0f, 1.0f));
    teeBorder->translate(-5.0f, pathY + slopeH + 0.02f, -1.0f);
    teeBorder->setup();
    addMesh(teeBorder);

    // ─── CUP & FLAG ───────────────────────────────────────────────────────────
    Mesh* cup = new Mesh(Mesh::makeCylinder(0.3f, 0.1f, 16, 0.04f, 0.04f, 0.04f, 1.0f));
    cup->translate(8.0f, pathY + 0.32f, 4.0f);
    cup->setup();
    addMesh(cup);

    Mesh* flagPole = new Mesh(Mesh::makeCylinder(0.04f, 1.2f, 8, 0.68f, 0.68f, 0.72f, 1.0f));
    flagPole->translate(8.0f, pathY + 0.95f, 4.0f);
    flagPole->setup();
    addMesh(flagPole);

    Mesh flagBanner = Mesh::makeTriangularPrism(0.5f, 0.05f, 0.3f, 0.90f, 0.10f, 0.10f, 1.0f);
    flagBanner.rotate(90.0f, 1.0f, 0.0f, 0.0f);
    flagBanner.translate(8.0f, pathY + 1.4f, 4.0f);
    flagBanner.setup();
    addMesh(new Mesh(std::move(flagBanner)));

    // ─── RAMP CONNECTOR ───────────────────────────────────────────────────────
    const float horizTopY = pathY + FH * 0.5f;
    const float hillTopY  = pathY + FH * 0.5f + slopeH;
    // Mesh ramp = Mesh::makeRamp(-5.0f, horizTopY, 2.0f, -5.0f, hillTopY, -1.0f, FW);
    // addMesh(new Mesh(std::move(ramp)));

    // ─── ROCK ON THE FAIRWAY ──────────────────────────────────────────────────
    // Placed mid-fairway as an obstacle — centre of horizontal section
    Mesh* fairwayRock = new Mesh(Mesh::makeHill(0.5f, 0.4f, 1.0f, 0.9f, 0.6f, C_GREY));
    fairwayRock->translate(3.0f, pathY + 0.3f, 4.0f);
    fairwayRock->setup();
    addMesh(fairwayRock);

    // ─── FLOWERS — straight line along Z=2.0, evenly spaced along X ──────────
    // Horizontal fairway spans X: -8 to +10. Place 8 flowers evenly across it.
    // All at the same Z=2.0 (front edge of fairway) so they form a straight row.
    const float flowerY  = pathY + 0.05f;
    const float flowerZ  = 2.0f;                    // single Z — straight line
    const float flowerX0 = -7.0f;
    const float flowerXStep = 2.0f;
    for (int i = 0; i < 8; i++) {
        float fx = flowerX0 + i * flowerXStep;

        Mesh* stem = new Mesh(Mesh::makeCylinder(0.04f, 0.25f, 6, 0.20f, 0.55f, 0.20f, 1.0f));
        stem->translate(fx, flowerY, flowerZ);
        stem->setup();
        addMesh(stem);

        Mesh* flower = new Mesh(Mesh::makeCylinder(0.12f, 0.06f, 8, C_FLOWER_RED));
        flower->translate(fx, flowerY + 0.15f, flowerZ);
        flower->setup();
        addMesh(flower);

        Mesh* center = new Mesh(Mesh::makeCylinder(0.05f, 0.08f, 6, C_FLOWER_YELLOW));
        center->translate(fx, flowerY + 0.16f, flowerZ);
        center->setup();
        addMesh(center);
    }

    // ─── LAMP POSTS — straight line along Z=6.0, evenly spaced along X ───────
    // Same X spacing as flowers, opposite Z edge of fairway (Z=6.0).
    const float lightY  = pathY;
    const float lightZ  = 6.0f;                     // single Z — straight line
    const float lightX0 = -7.0f;
    const float lightXStep = 2.0f;
    for (int i = 0; i < 7; i++) {
        float lx = lightX0 + i * lightXStep;

        Mesh* post = new Mesh(Mesh::makeCylinder(0.06f, 0.4f, 6, 0.50f, 0.40f, 0.30f, 1.0f));
        post->translate(lx, lightY + 0.2f, lightZ);
        post->setup();
        addMesh(post);

        Mesh* lamp = new Mesh(Mesh::makeCylinder(0.12f, 0.08f, 8, 0.95f, 0.90f, 0.60f, 1.0f));
        lamp->translate(lx, lightY + 0.42f, lightZ);
        lamp->setup();
        addMesh(lamp);
    }

    // ─── DINOSAUR FOOTPRINTS ──────────────────────────────────────────────────
    const float printY = SY + groundHeight + 0.03f;
    float prints[6][2] = {
        {-7.0f, 2.5f}, {-5.0f, 2.8f}, {-3.0f, 3.0f},
        {-1.0f, 3.2f}, {1.0f,  3.5f}, {3.0f,  3.8f}
    };
    for (int i = 0; i < 6; i++) {
        Mesh* print = new Mesh(Mesh::makeCylinder(0.22f, 0.04f, 12, 0.08f, 0.25f, 0.08f, 1.0f));
        print->translate(prints[i][0], printY, prints[i][1]);
        print->setup();
        addMesh(print);
    }

    // ─── DECORATIVE ROCKS (off-fairway) ───────────────────────────────────────
    const float rockY = SY + groundHeight + 0.1f;
    float rocks[6][2] = {
        {-10.0f,  5.0f}, {-9.0f, -5.0f}, {10.0f, -6.0f},
        { 12.0f,  1.0f}, {-11.0f,-2.0f}, { 9.0f, -3.0f}
    };
    for (int i = 0; i < 6; i++) {
        Mesh* rock = new Mesh(Mesh::makeHill(0.6f, 0.6f, 1.0f, 1.0f, 0.5f, C_GREY));
        rock->translate(rocks[i][0], rockY, rocks[i][1]);
        rock->setup();
        addMesh(rock);
    }

    // ─── BUSHES ───────────────────────────────────────────────────────────────
    const float bushY = SY + groundHeight;
    float bushes[6][2] = {
        {-12.0f,  4.0f}, {12.0f,  5.0f}, {13.0f, -4.0f},
        {-13.0f, -3.0f}, {-8.0f,  6.0f}, { 8.0f, -5.0f}
    };
    for (int i = 0; i < 6; i++) {
        Mesh* bush = new Mesh(Mesh::makeCone(0.5f, 0.6f, 10, 0.10f, 0.55f, 0.10f, 1.0f));
        bush->translate(bushes[i][0], bushY + 0.3f, bushes[i][1]);
        bush->setup();
        addMesh(bush);

        Mesh* bushTop = new Mesh(Mesh::makeCone(0.35f, 0.4f, 8, 0.12f, 0.60f, 0.12f, 1.0f));
        bushTop->translate(bushes[i][0], bushY + 0.65f, bushes[i][1]);
        bushTop->setup();
        addMesh(bushTop);
    }
}

Hole1::~Hole1()
{
    for (int i = 0; i < meshCount; i++)
        delete meshes[i];
}

void Hole1::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole1::draw(GLuint shaderProgram, bool wireframe,
                 const glm::vec3& lightDir,
                 const glm::vec3& lightColor,
                 const glm::vec3& camPos)
{
    glUseProgram(shaderProgram);
    GLint locLD = glGetUniformLocation(shaderProgram, "uLightDir");
    GLint locVP = glGetUniformLocation(shaderProgram, "uViewPos");
    GLint locLC = glGetUniformLocation(shaderProgram, "uLightColor");
    if (locLD != -1) glUniform3fv(locLD, 1, glm::value_ptr(glm::normalize(lightDir)));
    if (locVP != -1) glUniform3fv(locVP, 1, glm::value_ptr(camPos));
    if (locLC != -1) glUniform3fv(locLC, 1, glm::value_ptr(lightColor));

    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->draw(shaderProgram, wireframe);
}

void Hole1::drawDepth(GLuint depthShader)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->drawDepth(depthShader);
}

void Hole1::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->translate(x, y, z);
}

void Hole1::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->rotate(angle, ax, ay, az);
}