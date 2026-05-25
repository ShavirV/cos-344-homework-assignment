#include "GlobalWater.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

// =============================================================================
// Shader and texture paths
// =============================================================================
// All bodies share the same shader and normal-map textures.
// Adjust these once if your directory layout differs.
static const char* WATER_VERT  = "src/shaders/water.vert";
static const char* WATER_FRAG  = "src/shaders/water.frag";
static const char* NORMAL_MAP1 = "src/assets/textures/water_normal.bmp";
static const char* NORMAL_MAP2 = "src/assets/textures/water_normal2.bmp";
static const char* FOAM_MAP    = "src/assets/textures/water_foam.bmp";

// =============================================================================
// World-space positions for each water body
// =============================================================================
// All coordinates match the hole layout documented in main.cpp.
// Y values are set so water sits just below the ground slab (0.5 tall),
// appearing in every gap and depression without clipping through solid land.

// ── [0] Sea level — full-course underlay ─────────────────────────────────────
// Ground slab: makeCuboid(400, 0.5, 150), its top face is at Y=+0.25 when
// translated to Y=0 (ground.translate not called, sits at origin).
// Sea sits at Y=-0.60, safely below ground top (0.25) and bottom (-0.25).
// The large sheet ensures it is visible in every hole depression.
static const float SEA_Y     = -0.60f;
static const float SEA_W     = 420.0f;   // slightly wider than ground slab
static const float SEA_L     = 160.0f;   // slightly longer
static const int   SEA_RES   = 96;       // high resolution for large surface
static const float SEA_CX    =   0.0f;
static const float SEA_CZ    =   0.0f;

static const float H6_Y     = 0.00f;
static const float H6_W     = 8.0f;   
static const float H6_L     = 16.4f;   
static const int   H6_RES   = 6;      
static const float H6_CX    =   -5.0f;
static const float H6_CZ    =   -50.0f;


// =============================================================================
// Shared colour palettes
// =============================================================================
// Deep ocean blue for the sea level sheet.
static const glm::vec3 SEA_SHALLOW  = { 0.18f, 0.46f, 0.56f };
static const glm::vec3 SEA_DEEP     = { 0.02f, 0.10f, 0.26f };

// Warm sulphurous tint for the volcano lake.
static const glm::vec3 VLAKE_SHALLOW = { 0.35f, 0.42f, 0.28f };
static const glm::vec3 VLAKE_DEEP    = { 0.08f, 0.18f, 0.10f };

// Fresher river / stream tones.
static const glm::vec3 RIVER_SHALLOW = { 0.28f, 0.56f, 0.52f };
static const glm::vec3 RIVER_DEEP    = { 0.04f, 0.18f, 0.28f };

// Calm still-pond tones (slightly more blue-green than the river).
static const glm::vec3 POND_SHALLOW  = { 0.22f, 0.50f, 0.54f };
static const glm::vec3 POND_DEEP     = { 0.03f, 0.14f, 0.26f };

// =============================================================================
// makeBody — private static factory
// =============================================================================
WaterMesh* GlobalWater::makeBody(
    float width, float length, int res,
    float centreX, float centreY, float centreZ,
    float rotYDeg,
    glm::vec2 d0, float a0, float w0,
    glm::vec2 d1, float a1, float w1,
    glm::vec2 d2, float a2, float w2,
    glm::vec2 d3, float a3, float w3,
    float scroll0, float scroll1,
    glm::vec3 colorShallow,
    glm::vec3 colorDeep,
    float foamThreshold)
{
    WaterMesh* m = new WaterMesh();

    m->planeWidth     = width;
    m->planeLength    = length;
    m->gridResolution = res;

    m->waves[0] = { d0, a0, w0 };
    m->waves[1] = { d1, a1, w1 };
    m->waves[2] = { d2, a2, w2 };
    m->waves[3] = { d3, a3, w3 };

    m->scrollSpeed0  = scroll0;
    m->scrollSpeed1  = scroll1;
    m->colorShallow  = colorShallow;
    m->colorDeep     = colorDeep;
    m->foamThreshold = foamThreshold;

    // Build the model matrix: translate to world centre, then rotate around Y.
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(centreX, centreY, centreZ));
    if (rotYDeg != 0.0f)
        model = glm::rotate(model,
                            glm::radians(rotYDeg),
                            glm::vec3(0.0f, 1.0f, 0.0f));
    m->modelMatrix = model;

    m->init(WATER_VERT, WATER_FRAG, NORMAL_MAP1, NORMAL_MAP2, FOAM_MAP);

    return m;
}

GlobalWater::GlobalWater()
    : seaLevel(nullptr),
      camView(glm::mat4(1.0f)),
      camProj(glm::mat4(1.0f)),
      camPos(0.0f)
{
    // [0] Sea level
    // Large gently-undulating ocean sheet. Low amplitude, long wavelengths,
    // very slow scroll so it reads as a calm sea rather than a swimming pool.
    seaLevel = makeBody(
        SEA_W, SEA_L, SEA_RES,
        SEA_CX, SEA_Y, SEA_CZ,
        0.0f,
        // waves: direction, amplitude, wavelength
        { 1.0f,  0.0f}, 0.06f, 18.0f,   // primary swell
        { 0.0f,  1.0f}, 0.04f, 14.0f,   // cross-swell
        { 0.3f,  0.5f}, 0.13f,  9.0f,   // diagonal ripple
        {-0.5f,  0.5f}, 0.02f,  6.0f,   // counter-diagonal
        0.018f, 0.012f,                   // slow scroll
        SEA_SHALLOW, SEA_DEEP,
        0.22f);                           // gentle foam at wave crests

    //hole

    h6Water = makeBody(
        H6_W, H6_L, H6_RES,
        H6_CX, H6_Y, H6_CZ,
        0.0f,
        // waves: direction, amplitude, wavelength
        { 0.02f,  0.0f}, 0.03f, 26.0f,   // primary swell (reduced amplitude, longer waves)
        { 0.0f,  1.2f}, 0.025f, 22.0f,  // cross-swell
        { 0.07f,  0.07f}, 0.015f, 18.0f,  // diagonal ripple (subtle texture only)
        {-0.05f,  0.05f}, 0.010f, 14.0f,  // counter-diagonal (barely visible detail)

        0.010f, 0.006f,                 // slower scroll (key for “calm” feel)
        RIVER_SHALLOW, RIVER_DEEP,
        0.22f); 

    // Populate the iteration array in draw order
    // (sea first — furthest back; ponds last — nearest foreground).
    bodies[0] = seaLevel;
    bodies[1] = h6Water;
}

// Destructor
GlobalWater::~GlobalWater()
{
    for (int i = 0; i < BODY_COUNT; i++)
    {
        if (bodies[i]){
            delete bodies[i];
            bodies[i] = nullptr;
        }
    }
    // Individual pointers are aliases into bodies[] — no double-delete needed.
    seaLevel    = nullptr;
    h6Water = nullptr;
}

// =============================================================================
// update
// =============================================================================
void GlobalWater::update(float deltaTime)
{
    for (int i = 0; i < BODY_COUNT; i++)
        if (bodies[i]) bodies[i]->update(deltaTime);
}

void GlobalWater::setCameraData(const glm::mat4& view,
                                const glm::mat4& proj,
                                const glm::vec3& pos)
{
    camView = view;
    camProj = proj;
    camPos  = pos;
}

void GlobalWater::draw(GLuint shaderProgram, bool wireframe,
                       glm::vec3 lightDir,
                       glm::vec3 lightColor)
{
    // Normalise lightDir defensively — caller may pass un-normalised.
    glm::vec3 normDir = glm::normalize(lightDir);

    // Water is drawn after all opaque geometry.  Each WaterMesh handles its
    // own depth-mask management internally.
    for (int i = 0; i < BODY_COUNT; i++)
        if (bodies[i])
            bodies[i]->draw(camView, camProj, camPos, normDir, lightColor);
}

void GlobalWater::translate(float x, float y, float z)
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    for (int i = 0; i < BODY_COUNT; i++)
        if (bodies[i])
            bodies[i]->modelMatrix = T * bodies[i]->modelMatrix;
}