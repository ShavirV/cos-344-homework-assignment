#include "Hole10.h"
#include <cmath>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Height function — drives the entire surface as one continuous terrain.
//
// nx : [0,1] normalised position along X (0=left end, 1=right end)
// nz : [0,1] normalised position along Z (0=front, 1=back)
//
// The terrain is divided into three X bands:
//   [0.00 – 0.38]  left flat section  y = HIGH
//   [0.38 – 0.58]  ramp               y = lerp(HIGH, LOW, t)
//   [0.58 – 1.00]  right flat section y = LOW
//
// A gentle crown (higher in the centre of Z) is added across the full surface
// to give the fairway a slight camber.
// ─────────────────────────────────────────────────────────────────────────────
static float hole10Height(float nx, float nz, float s)
{
    const float HIGH  = 4.0f   * s;
    const float LOW   = 0.35f  * s;
    const float CROWN = 0.18f  * s;   // max camber at Z centre

    float base;
    if (nx < 0.38f)
        base = HIGH;
    else if (nx > 0.58f)
        base = LOW;
    else
    {
        float t = (nx - 0.38f) / 0.20f;
        // Smooth-step for a gentler ramp than a straight lerp.
        t    = t * t * (3.0f - 2.0f * t);
        base = HIGH + (LOW - HIGH) * t;
    }

    // Crown: cosine across Z so edges are slightly lower than the centre.
    float crown = CROWN * cosf((nz - 0.5f) * (float)M_PI);

    return base + crown;
}

// ─────────────────────────────────────────────────────────────────────────────
// Width mask — controls how wide the fairway is at each X position.
// Returns a half-width in Z world units (before scaling).
// Left end: wide (8).  Ramp throat: narrow (4).  Right end: wide (7.5).
// ─────────────────────────────────────────────────────────────────────────────
static float hole10HalfWidth(float nx, float s)
{
    // Left section tapers from 8 → 4 toward the ramp.
    if (nx < 0.38f)
    {
        float t = nx / 0.38f;
        return (8.0f - 4.0f * t) * s;
    }
    // Ramp throat stays at 4, then opens to 5 by the end of the ramp.
    if (nx < 0.58f)
    {
        float t = (nx - 0.38f) / 0.20f;
        return (4.0f + 1.0f * t) * s;
    }
    // Right section opens from 5 → 7.5 with a slight S-curve.
    float t = (nx - 0.58f) / 0.42f;
    t = t * t * (3.0f - 2.0f * t);
    return (5.0f + 2.5f * t) * s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Hole10
// ─────────────────────────────────────────────────────────────────────────────

Hole10::Hole10(float scale)
    : meshCount(0), s(scale)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Main terrain surface ──────────────────────────────────────────────────
    const float TERRAIN_LEN = 48.0f * s;
    const float TERRAIN_WID = 18.0f * s;
    const int   SEGS_X      = 48;
    const int   SEGS_Z      = 18;

    float sc = s;

 auto heightFn = [sc](float nx, float nz) -> float
{
    float nxMirror = 1.0f - nx;   // flip X so ramp runs right-to-left

    float hw   = hole10HalfWidth(nxMirror, sc);
    float zPos = (nz - 0.5f) * 18.0f * sc;

    if (fabsf(zPos) > hw)
        return -2.0f * sc;

    return hole10Height(nxMirror, nz, sc);
};

    Mesh* terrain = new Mesh(Mesh::makeHeightmapTerrain(
        TERRAIN_LEN, TERRAIN_WID,
        SEGS_X, SEGS_Z,
        heightFn,
        0.2f, 0.62f, 0.22f, 1.0f));
    terrain->translate(0.0f, 0.0f, 0.0f);
    terrain->setup();
    addMesh(terrain);

    // ── Left-section rocks (mirrored) ────────────────────────────────────────
    addRock( 22.0f*s,  3.0f*s,   9.0f*s, 1.5f*s);
    addRock( 20.0f*s,  3.0f*s,  -9.0f*s, 1.5f*s);
    addRock( 18.0f*s,  3.0f*s,   7.0f*s, 1.3f*s);
    addRock( 16.0f*s,  3.0f*s,  -7.5f*s, 1.2f*s);
    addRock( 14.0f*s,  3.0f*s,  10.0f*s, 1.0f*s);

    // ── Ramp-side rocks (mirrored) ────────────────────────────────────────────
    addRock(  4.0f*s,  1.5f*s,   6.0f*s, 1.0f*s);
    addRock(  2.0f*s,  1.5f*s,  -6.0f*s, 1.0f*s);

    // ── Right-section rocks (mirrored) ───────────────────────────────────────
    addRock(-14.0f*s, -0.65f*s,  9.0f*s, 2.0f*s);
    addRock(-18.0f*s, -0.65f*s, -8.5f*s, 2.0f*s);

    // ── Cup (mirrored) ────────────────────────────────────────────────────────
    Mesh* cup = new Mesh(Mesh::makeCylinder(
        0.8f*s, 0.15f*s, 32,
        0.08f, 0.08f, 0.08f, 1.0f));
    cup->translate( 20.0f*s, 4.35f*s,  1.0f*s);
    cup->setup();
    addMesh(cup);

    // ── Flag pole (mirrored) ──────────────────────────────────────────────────
    Mesh* pole = new Mesh(Mesh::makeCylinder(
        0.08f * s, 6.0f * s, 8,
        0.9f, 0.9f, 0.9f, 1.0f));
    pole->translate( 20.0f*s, 7.35f*s,  1.0f*s);
    pole->setup();
    addMesh(pole);

    // ── Flag (mirrored) ───────────────────────────────────────────────────────
    Mesh* flag1 = new Mesh(Mesh::makeCuboid(
        1.5f * s, 0.8f * s, 0.3f * s,
        1.0f, 0.1f, 0.1f, 1.0f));
    flag1->translate( 19.25f*s, 9.85f*s,  1.0f*s);
    flag1->setup();
    addMesh(flag1);

    // ── Extra rocks (mirrored) ────────────────────────────────────────────────
    addRock( 17.0f*s, 4.35f*s,  1.0f*s, 1.8f*s);
    addRock( 17.0f*s, 4.35f*s,  4.0f*s, 1.8f*s);
}

Hole10::~Hole10()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole10::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole10::addRock(float x, float y, float z, float scale)
{
    Mesh* r = new Mesh(Mesh::makeRock(scale));
    r->translate(x, y, z);
    r->setup();
    addMesh(r);
}

void Hole10::draw(GLuint shaderProgram, bool wireframe,
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

void Hole10::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole10::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}