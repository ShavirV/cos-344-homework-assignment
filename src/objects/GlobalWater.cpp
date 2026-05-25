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

// ── [1] Volcano lake — crater surroundings ────────────────────────────────────
// Volcano centre: (0, 20, -10) in main.cpp (cone translated there).
// The lake is a flat plane at the volcano base, visible as a hot-spring ring.
static const float VLAKE_Y   = -0.30f;
static const float VLAKE_W   =  44.0f;
static const float VLAKE_L   =  44.0f;
static const int   VLAKE_RES =  48;
static const float VLAKE_CX  =   0.0f;
static const float VLAKE_CZ  = -10.0f;

// ── [2] West river — through holes 5, 6, 7, 8, 9 ────────────────────────────
// Holes 5–9 all cluster around Z=-8 to Z=-50, X=-175 to X=-40.
// A long east–west strip covers this corridor.
// Rotated 0° (runs along X).  Made slightly narrower than the gaps between
// holes so it appears as a river channel, not a flood.
static const float RIVER_Y   = -0.25f;
static const float RIVER_W   = 140.0f;  // length along X
static const float RIVER_L   =   9.0f;  // width along Z
static const int   RIVER_RES =  32;
static const float RIVER_CX  = -105.0f; // midpoint of X=-175...-40 range
static const float RIVER_CZ  =  -20.0f; // between holes 5-6 (Z=-16) and 7-9 (Z=-35)
static const float RIVER_ROT =   0.0f;  // aligned along X axis

// ── [3] East pond — near holes 10-11 ─────────────────────────────────────────
// Hole 10 at X=+80, Z=-10.  Hole 11 at X=+110, Z=+20.
// Pond sits between them.
static const float EPOND_Y   = -0.22f;
static const float EPOND_W   =  42.0f;
static const float EPOND_L   =  42.0f;
static const int   EPOND_RES =  48;
static const float EPOND_CX  =  95.0f;
static const float EPOND_CZ  = -10.0f;

// ── [4] North pond — decorative, northern edge ───────────────────────────────
// Sits between the northernmost holes (H1-H4) and the volcano.
// Adds visual interest to the open ground area at Z=+20..+40.
static const float NPOND_Y   = -0.22f;
static const float NPOND_W   =  32.0f;
static const float NPOND_L   =  32.0f;
static const int   NPOND_RES =  48;
static const float NPOND_CX  = -20.0f;
static const float NPOND_CZ  =  22.0f;

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

// =============================================================================
// Constructor
// =============================================================================
GlobalWater::GlobalWater()
    : seaLevel(nullptr),
      volcanoLake(nullptr),
      westRiver(nullptr),
      eastPond(nullptr),
      northPond(nullptr),
      camView(glm::mat4(1.0f)),
      camProj(glm::mat4(1.0f)),
      camPos(0.0f)
{
    // ── [0] Sea level ─────────────────────────────────────────────────────────
    // Large gently-undulating ocean sheet. Low amplitude, long wavelengths,
    // very slow scroll so it reads as a calm sea rather than a swimming pool.
    seaLevel = makeBody(
        SEA_W, SEA_L, SEA_RES,
        SEA_CX, SEA_Y, SEA_CZ,
        0.0f,
        // waves: direction, amplitude, wavelength
        { 1.0f,  0.0f}, 0.06f, 18.0f,   // primary swell
        { 0.0f,  1.0f}, 0.04f, 14.0f,   // cross-swell
        { 0.7f,  0.7f}, 0.03f,  9.0f,   // diagonal ripple
        {-0.5f,  0.5f}, 0.02f,  6.0f,   // counter-diagonal
        0.018f, 0.012f,                   // slow scroll
        SEA_SHALLOW, SEA_DEEP,
        0.22f);                           // gentle foam at wave crests

    // ── [1] Volcano lake ──────────────────────────────────────────────────────
    // Slightly warmer, more turbulent than the ponds — geothermal activity.
    // Moderate foam threshold gives a steaming, unsettled appearance.
    volcanoLake = makeBody(
        VLAKE_W, VLAKE_L, VLAKE_RES,
        VLAKE_CX, VLAKE_Y, VLAKE_CZ,
        0.0f,
        { 1.0f,  0.2f}, 0.10f,  6.0f,
        {-0.3f,  1.0f}, 0.08f,  4.5f,
        { 0.6f, -0.6f}, 0.06f,  3.0f,
        {-0.8f, -0.4f}, 0.04f,  2.0f,
        0.10f, 0.07f,
        VLAKE_SHALLOW, VLAKE_DEEP,
        0.10f);                           // more foam — turbulent hot spring

    // ── [2] West river ────────────────────────────────────────────────────────
    // Higher scroll speed to simulate downstream flow from north to south.
    // Wave directions biased along −Z (southward flow).
    // The strip is aligned along X (rot=0), flow direction is along Z.
    westRiver = makeBody(
        RIVER_W, RIVER_L, RIVER_RES,
        RIVER_CX, RIVER_Y, RIVER_CZ,
        RIVER_ROT,
        { 0.1f, -1.0f}, 0.12f,  5.0f,   // main downstream flow
        { 0.2f, -1.0f}, 0.07f,  3.0f,   // secondary current
        {-0.1f, -1.0f}, 0.05f,  2.0f,   // edge eddies
        { 0.0f, -0.8f}, 0.04f,  1.5f,   // surface ripple
        0.28f, 0.18f,                     // fast scroll — flowing river
        RIVER_SHALLOW, RIVER_DEEP,
        0.06f);                           // light foam at rapids

    // ── [3] East pond ─────────────────────────────────────────────────────────
    // Calm standing water. Very small waves, slow scroll.
    eastPond = makeBody(
        EPOND_W, EPOND_L, EPOND_RES,
        EPOND_CX, EPOND_Y, EPOND_CZ,
        0.0f,
        { 1.0f,  0.0f}, 0.04f,  6.0f,
        { 0.0f,  1.0f}, 0.03f,  5.0f,
        { 0.7f,  0.7f}, 0.02f,  3.5f,
        {-0.5f,  0.5f}, 0.015f, 2.5f,
        0.03f, 0.02f,
        POND_SHALLOW, POND_DEEP,
        0.18f);                           // almost no foam — still pond

    // ── [4] North pond ────────────────────────────────────────────────────────
    // Even calmer than east pond — ornamental water feature.
    northPond = makeBody(
        NPOND_W, NPOND_L, NPOND_RES,
        NPOND_CX, NPOND_Y, NPOND_CZ,
        0.0f,
        { 1.0f,  0.0f}, 0.03f,  7.0f,
        { 0.0f,  1.0f}, 0.02f,  5.5f,
        { 0.6f,  0.6f}, 0.015f, 4.0f,
        {-0.4f,  0.4f}, 0.010f, 3.0f,
        0.02f, 0.015f,
        POND_SHALLOW, POND_DEEP,
        0.20f);

    // Populate the iteration array in draw order
    // (sea first — furthest back; ponds last — nearest foreground).
    bodies[0] = seaLevel;
    bodies[1] = volcanoLake;
    bodies[2] = westRiver;
    bodies[3] = eastPond;
    bodies[4] = northPond;
}

// =============================================================================
// Destructor
// =============================================================================
GlobalWater::~GlobalWater()
{
    for (int i = 0; i < BODY_COUNT; i++)
    {
        delete bodies[i];
        bodies[i] = nullptr;
    }
    // Individual pointers are aliases into bodies[] — no double-delete needed.
    seaLevel    = nullptr;
    volcanoLake = nullptr;
    westRiver   = nullptr;
    eastPond    = nullptr;
    northPond   = nullptr;
}

// =============================================================================
// update
// =============================================================================
void GlobalWater::update(float deltaTime)
{
    for (int i = 0; i < BODY_COUNT; i++)
        if (bodies[i]) bodies[i]->update(deltaTime);
}

// =============================================================================
// setCameraData
// =============================================================================
void GlobalWater::setCameraData(const glm::mat4& view,
                                const glm::mat4& proj,
                                const glm::vec3& pos)
{
    camView = view;
    camProj = proj;
    camPos  = pos;
}

// =============================================================================
// draw
// =============================================================================
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

// =============================================================================
// translate
// =============================================================================
// Shifts every body's world-space position by (x,y,z).
// Implemented by pre-multiplying each body's modelMatrix with a translation,
// matching the applyMatrix pattern used by Hole16/Hole18.
void GlobalWater::translate(float x, float y, float z)
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    for (int i = 0; i < BODY_COUNT; i++)
        if (bodies[i])
            bodies[i]->modelMatrix = T * bodies[i]->modelMatrix;
}