#include "Hole12.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

Hole12::Hole12(float s)
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Hourglass fairway ─────────────────────────────────────────────────────
    const int   SEGS    = 20;
    const float LENGTH  = 28.0f * s;
    const float W_END   = 15.0f * s;
    const float W_MID   =  6.0f * s;
    const float SEG_LEN = LENGTH / SEGS;
    const float THICK   =  0.2f * s;

    auto widthAt = [&](float t) -> float {
        return W_MID + (W_END - W_MID) * 0.5f * (1.0f + cosf(2.0f * (float)M_PI * t));
    };

    for (int i = 0; i < SEGS; i++)
    {
        float t0 = (float)i       / SEGS;
        float t1 = (float)(i + 1) / SEGS;
        float tM = (t0 + t1) * 0.5f;
        float wM = widthAt(tM);

        float xCentre = -LENGTH * 0.5f + (tM * LENGTH);

        Mesh* strip = new Mesh(Mesh::makeCuboid(
            SEG_LEN, THICK, wM,
            0.2f, 0.62f, 0.22f, 1.0f));
        strip->translate(xCentre, 0.0f, 0.0f);
        strip->setup();
        addMesh(strip);

        // ── Border strips ─────────────────────────────────────────────────────
        const float BH = 0.4f * s;
        const float BT = 0.4f * s;

        Mesh* bPos = new Mesh(Mesh::makeCuboid(
            SEG_LEN, BH, BT,
            0.9f, 0.9f, 0.9f, 1.0f));
        bPos->translate(xCentre, 0.0f, wM * 0.5f + BT * 0.5f);
        bPos->setup();
        addMesh(bPos);

        Mesh* bNeg = new Mesh(Mesh::makeCuboid(
            SEG_LEN, BH, BT,
            0.9f, 0.9f, 0.9f, 1.0f));
        bNeg->translate(xCentre, 0.0f, -(wM * 0.5f + BT * 0.5f));
        bNeg->setup();
        addMesh(bNeg);
    }

    // ── End caps ──────────────────────────────────────────────────────────────
    const float BH  = 0.4f * s;
    const float BT  = 0.5f * s;
    const float wL  = widthAt(0.0f);
    const float wR  = widthAt(1.0f);

    Mesh* capL = new Mesh(Mesh::makeCuboid(
        BT, BH, wL + 1.0f * s,
        0.9f, 0.9f, 0.9f, 1.0f));
    capL->translate(-LENGTH * 0.5f - BT * 0.5f, 0.0f, 0.0f);
    capL->setup();
    addMesh(capL);

    Mesh* capR = new Mesh(Mesh::makeCuboid(
        BT, BH, wR + 1.0f * s,
        0.9f, 0.9f, 0.9f, 1.0f));
    capR->translate(LENGTH * 0.5f + BT * 0.5f, 0.0f, 0.0f);
    capR->setup();
    addMesh(capR);

    // ── Cup ───────────────────────────────────────────────────────────────────
    Mesh* cup = new Mesh(Mesh::makeCylinder(
        0.5f*s, 0.3f*s, 32,
        0.05f, 0.05f, 0.05f, 1.0f));
    cup->translate(-11.0f*s, 0.2f*s, 0.0f);
    cup->setup();
    addMesh(cup);

  Mesh* pole = new Mesh(Mesh::makeCylinder(
    0.08f*s, 6.0f*s, 8,
    0.9f, 0.9f, 0.9f, 1.0f));
pole->translate(-11.0f*s, 3.2f*s, 0.0f);
pole->setup();
addMesh(pole);

// ── Flag ──────────────────────────────────────────────────────────────────
Mesh* flag1 = new Mesh(Mesh::makeCuboid(
    1.5f*s, 0.8f*s, 0.3f*s,
    1.0f, 0.1f, 0.1f, 1.0f));
flag1->translate(-10.25f*s, 5.8f*s, 0.0f);
flag1->setup();
addMesh(flag1);
}

Hole12::~Hole12()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole12::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole12::draw(GLuint shaderProgram, bool wireframe,
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
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole12::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole12::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}