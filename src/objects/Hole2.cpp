// =============================================================================
// Hole2.cpp  –  Dino Park Phuket | Hole 2 (Jungle Entry Zone)
//
// Course layout (top-down, +X right, +Z toward viewer, +Y up):
//
//   Simple sloping fairway: Tee mat at higher elevation, sloping down to hole
// =============================================================================

#include "Hole2.h"
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

// ─── Colours ──────────────────────────────────────────────────────────────────
#define C_GREEN      0.15f, 0.50f, 0.15f, 1.0f
#define C_FAIRWAY    0.18f, 0.72f, 0.22f, 1.0f
#define C_DARKGRASS  0.08f, 0.32f, 0.08f, 1.0f
#define C_SAND       0.93f, 0.85f, 0.60f, 1.0f
#define C_FLAGRED    0.90f, 0.10f, 0.10f, 1.0f
#define C_START      0.85f, 0.60f, 0.20f, 1.0f
#define C_BLACK      0.04f, 0.04f, 0.04f, 1.0f
#define C_METAL      0.68f, 0.68f, 0.72f, 1.0f
#define C_WHITE      0.96f, 0.96f, 0.96f, 1.0f
#define C_BROWN      0.38f, 0.24f, 0.12f, 1.0f

// ─── Helper ───────────────────────────────────────────────────────────────────
static Mesh* place(Mesh&& src, float x, float y, float z,
                   Mesh** pool, int& cnt, int maxCnt)
{
    Mesh* m = new Mesh(std::move(src));
    m->translate(x, y, z);
    m->setup();
    if (cnt < maxCnt) pool[cnt++] = m;
    return m;
}

// ---------------------------------------------------------------------------
// addMesh
// ---------------------------------------------------------------------------
void Hole2::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

// ---------------------------------------------------------------------------
// Constructor – builds all geometry
// ---------------------------------------------------------------------------
Hole2::Hole2() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. BASE GROUND (flat surface that covers the entire area)
    // ─────────────────────────────────────────────────────────────────────────
    const float groundY = -0.2f;
    const float groundHeight = 0.2f;

    // Base ground platform (30 x 17 as requested)
    place(Mesh::makeCuboid(30.0f, groundHeight, 17.0f, C_DARKGRASS),
        0.0f, groundY + groundHeight * 0.5f, 0.0f,
        meshes, meshCount, MAX_MESHES);

    // ─────────────────────────────────────────────────────────────────────────
    // 2. SLOPING FAIRWAY (tee area higher, slopes down to the hole)
    // ─────────────────────────────────────────────────────────────────────────
    
    // Tee area platform (raised)
    const float teeHeight = 1.2f;
    const float teeY = groundY + groundHeight + teeHeight * 0.5f;
    
    place(Mesh::makeCuboid(4.0f, teeHeight, 6.0f, C_GREEN),
        -10.0f, teeY, 0.0f,
        meshes, meshCount, MAX_MESHES);
    
    // Sloping fairway (ramp from tee area down to ground level)
    // FLIPPED: Starts at x=-8, y=0.0; ends at x=8, y=0.8
    {
        Mesh* ramp = new Mesh(Mesh::makeRamp(-8.0f, -0.02f, 0.0f, 8.0f, 0.3f, 0.0f, 6.0f));
        ramp->setRGBA(C_FAIRWAY);
        ramp->setup();
        addMesh(ramp);
    }
    
    // Approach area (flat before the green)
    place(Mesh::makeCuboid(6.0f, 0.2f, 8.0f, C_FAIRWAY),
        11.0f, groundY + groundHeight + 0.1f, 0.0f,
        meshes, meshCount, MAX_MESHES);
    
  

    // ─────────────────────────────────────────────────────────────────────────
    // 3. TEE MAT (bright yellow with white border)
    // ─────────────────────────────────────────────────────────────────────────
    const float matY = groundY + groundHeight + teeHeight + 0.05f;
    
    // Yellow tee mat
    Mesh teeMat = Mesh::makeCuboid(2.5f, 0.05f, 2.5f, 1.0f, 0.8f, 0.0f, 1.0f);
    teeMat.translate(-10.0f, matY, 0.0f);
    teeMat.setup();
    addMesh(new Mesh(std::move(teeMat)));

    // White border
    Mesh teeBorder = Mesh::makeCuboid(2.65f, 0.02f, 2.65f, 1.0f, 1.0f, 1.0f, 1.0f);
    teeBorder.translate(-10.0f, matY - 0.02f, 0.0f);
    teeBorder.setup();
    addMesh(new Mesh(std::move(teeBorder)));

    // ─────────────────────────────────────────────────────────────────────────
    // 4. HOLE AND FLAG
    // ─────────────────────────────────────────────────────────────────────────
    const float cupX = 13.0f, cupZ = 2.0f;
    const float cupY = groundY + groundHeight + 0.5f;
    
    // The cup (hole)
    Mesh* cup = new Mesh(Mesh::makeCylinder(0.22f, 0.15f, 12, C_BLACK));
    cup->translate(cupX, cupY, cupZ);
    cup->setup();
    addMesh(cup);
    
    
    
    // Flag pole
    Mesh* pole = new Mesh(Mesh::makeCylinder(0.05f, 2.5f, 8, C_METAL));
    pole->translate(cupX, cupY + 1.25f, cupZ);
    pole->setup();
    addMesh(pole);
    
    

    

    
    
    
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
Hole2::~Hole2()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

// ---------------------------------------------------------------------------
// draw
// ---------------------------------------------------------------------------
void Hole2::draw(GLuint shaderProgram, bool wireframe,
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

void Hole2::drawDepth(GLuint depthShader)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) meshes[i]->drawDepth(depthShader);
}

// ---------------------------------------------------------------------------
// translate – apply to all meshes
// ---------------------------------------------------------------------------
void Hole2::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) 
            meshes[i]->translate(x, y, z);
}

// ---------------------------------------------------------------------------
// rotate – apply to all meshes
// ---------------------------------------------------------------------------
void Hole2::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        if (meshes[i]) 
            meshes[i]->rotate(angle, ax, ay, az);
}