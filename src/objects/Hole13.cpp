// =============================================================================
// Hole13.cpp — "Volcanic Gateway"   COS344 Practical 2026
//
// Changes from the original:
//   1. Fairway mesh subdivided to 64×12 quads for smooth per-fragment lighting
//      on the sinusoidal incline.
//   2. Richer colour palette — ash whites, obsidian blacks, orange lava tones.
//   3. Lava pool caldera with animated glow (shader's uEmissive flag).
//   4. Pumice / ash terrain patches on the ground pad.
//   5. Obsidian ledge strips along the fairway edges.
//   6. Volcano.glb imported and placed at the back of the bend.
//   7. Secondary lava point-light warms the underside of nearby geometry.
//   8. All new shader uniforms (uEmissive, uEmissiveColor, uLavaLight*)
//      are looked up and set gracefully.
// =============================================================================

#include "Hole13.h"
#include "HoleMeshBuilder.h"
#include "GltfLoader.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>

using namespace holegeom;

// ── Internal lava pool mesh builder ─────────────────────────────────────────
// Separate from the procedural mesh so we can set uEmissive=1 just for it.
namespace {

void buildLavaPool(float cx, float y, float cz,
                   float rInner, float rOuter, int segs,
                   GLuint& vao, GLuint& vbo, GLuint& ebo, GLsizei& count)
{
    MeshBuilder mb;
    // Outer glowing lava surface (ring)
    addRing(mb, cx, y, cz, rInner, rOuter, segs,
            1.0f, 0.42f, 0.04f);
    // Inner very bright core
    addDisc(mb, cx, y + 0.01f, cz, rInner, segs,
            1.0f, 0.72f, 0.10f);
    // Lava-splash crags around the rim (small boulders in orange/red)
    const unsigned int SEEDS[8] = { 9001,9002,9003,9004,9005,9006,9007,9008 };
    for (int i = 0; i < 8; ++i) {
        float a  = float(i) / 8.0f * 6.28318530718f;
        float bx = cx + std::cos(a) * (rOuter * 0.88f);
        float bz = cz + std::sin(a) * (rOuter * 0.88f);
        float sz = 0.35f + (float)(SEEDS[i] % 7) * 0.05f;
        addBoulder(mb, bx, y + sz * 0.5f, bz, sz,
                   1.0f, 0.30f + (float)(i % 3) * 0.08f, 0.0f,
                   SEEDS[i]);
    }
    count = uploadMesh(mb, vao, vbo, ebo);
}

} // anon namespace

// ── Constructor / Destructor ─────────────────────────────────────────────────
Hole13::Hole13()
    : _vao(0), _vbo(0), _ebo(0), _indexCount(0),
      _lavaVao(0), _lavaVbo(0), _lavaEbo(0), _lavaIndexCount(0),
      _volcano(0), _program(0),
      _uMVP(-1), _uModel(-1), _uNormalMat(-1),
      _uLightDir(-1), _uLightColor(-1), _uAmbient(-1), _uViewPos(-1),
      _uEmissive(-1), _uEmissiveColor(-1),
      _uLavaLightPos(-1), _uLavaLightColor(-1), _uLavaLightIntensity(-1),
      _teePos(0.0f), _cupPos(0.0f),
      _sceneCenter(0.0f), _lavaPoolCenter(0.0f),
      _sceneRadius(50.0f)
{}

Hole13::~Hole13() { cleanup(); }

// ── init ─────────────────────────────────────────────────────────────────────
void Hole13::init(GLuint shaderProgram, const char* glbPath)
{
    _program = shaderProgram;

    // Standard uniforms
    _uMVP        = glGetUniformLocation(_program, "uMVP");
    _uModel      = glGetUniformLocation(_program, "uModel");
    _uNormalMat  = glGetUniformLocation(_program, "uNormalMat");
    _uLightDir   = glGetUniformLocation(_program, "uLightDir");
    _uLightColor = glGetUniformLocation(_program, "uLightColor");
    _uAmbient    = glGetUniformLocation(_program, "uAmbient");
    _uViewPos    = glGetUniformLocation(_program, "uViewPos");

    // New emissive / lava-light uniforms
    _uEmissive            = glGetUniformLocation(_program, "uEmissive");
    _uEmissiveColor       = glGetUniformLocation(_program, "uEmissiveColor");
    _uLavaLightPos        = glGetUniformLocation(_program, "uLavaLightPos");
    _uLavaLightColor      = glGetUniformLocation(_program, "uLavaLightColor");
    _uLavaLightIntensity  = glGetUniformLocation(_program, "uLavaLightIntensity");

    build();

    // Load volcano asset
    _volcano = gltfloader::loadGLB(glbPath);
    if (!_volcano) {
        std::cerr << "[Hole13] Warning: failed to load " << glbPath
                  << " — volcano will not render.\n";
    }
}

// ── build ────────────────────────────────────────────────────────────────────
void Hole13::build()
{
    MeshBuilder mb;

    // ── Palette ──────────────────────────────────────────────────────────────
    //   Volcanic zone colours: less saturated greens, lava contrast,
    //   obsidian/basalt darks, chalky ash whites.
    const float turfA[3]    = { 0.20f, 0.58f, 0.18f }; // fresh grass (tee)
    const float turfB[3]    = { 0.15f, 0.48f, 0.14f }; // scorched green (top)
    const float turfC[3]    = { 0.18f, 0.52f, 0.16f }; // mid-green stripe
    const float rockC[3]    = { 0.18f, 0.16f, 0.15f }; // basalt dark
    const float obsidian[3] = { 0.08f, 0.07f, 0.09f }; // obsidian ledge
    const float pumice[3]   = { 0.62f, 0.58f, 0.54f }; // pumice / ash
    const float ashGrey[3]  = { 0.44f, 0.42f, 0.40f }; // ash ground
    const float boneC[3]    = { 0.88f, 0.84f, 0.76f }; // chalky bone
    const float concrete[3] = { 0.70f, 0.68f, 0.65f }; // tee pad
    const float darkLava[3] = { 0.10f, 0.08f, 0.06f }; // cooled lava ground

    // ── Layout ───────────────────────────────────────────────────────────────
    const float legXMin = -7.0f, legXMax = 7.0f;
    const float legZ0   =  0.0f, legZ1   = 50.0f;
    const float legYTop =  5.0f;
    const float bendX0  = -7.0f, bendX1  = 40.0f;
    const float bendZ0  = 50.0f, bendZ1  = 75.0f;   // extended

    // ── Large volcanic ground pad ────────────────────────────────────────────
    // Subdivided 4×4 grid of dark lava patches for visual variety.
    const float padXMin = -25.0f, padXMax = 58.0f;
    const float padZMin = -10.0f, padZMax = 90.0f;
    const int   padDivX = 4, padDivZ = 5;
    for (int pj = 0; pj < padDivZ; ++pj) {
        for (int pi = 0; pi < padDivX; ++pi) {
            float x0 = padXMin + (padXMax - padXMin) * float(pi)     / padDivX;
            float x1 = padXMin + (padXMax - padXMin) * float(pi + 1) / padDivX;
            float z0 = padZMin + (padZMax - padZMin) * float(pj)     / padDivZ;
            float z1 = padZMin + (padZMax - padZMin) * float(pj + 1) / padDivZ;
            // Alternate between dark lava and ash grey for patchwork look
            float t  = float((pi + pj) & 1) * 0.12f;
            addFlatQuad(mb,
                        x0, -0.05f, z0,
                        x0, -0.05f, z1,
                        x1, -0.05f, z1,
                        x1, -0.05f, z0,
                        darkLava[0] + t, darkLava[1] + t, darkLava[2] + t);
        }
    }

    // Ash-field patches scattered around the hole perimeter
    struct AshPatch { float x0,z0,x1,z1; };
    AshPatch ashPatches[] = {
        { -25.0f, 5.0f, -10.0f, 25.0f },
        { -22.0f, 30.0f, -9.0f, 48.0f },
        {  45.0f, 10.0f, 57.0f, 45.0f },
        {  42.0f, 50.0f, 57.0f, 75.0f },
        { -18.0f, 55.0f, -8.0f, 78.0f },
    };
    for (int i = 0; i < 5; ++i) {
        AshPatch& a = ashPatches[i];
        addFlatQuad(mb,
                    a.x0, -0.02f, a.z0,
                    a.x0, -0.02f, a.z1,
                    a.x1, -0.02f, a.z1,
                    a.x1, -0.02f, a.z0,
                    ashGrey[0], ashGrey[1], ashGrey[2]);
    }

    // ── Sinusoidal height function ────────────────────────────────────────────
    auto legHeight = [&](float z) -> float {
        if (z <= legZ0) return 0.0f;
        if (z >= legZ1) return legYTop;
        float t = (z - legZ0) / (legZ1 - legZ0);
        return legYTop * (1.0f - std::cos(t * 3.14159265f)) * 0.5f;
    };

    // ── Straight-leg fairway (higher-res: 64 rows × 12 cols) ─────────────────
    const int gridZ = 64, gridX = 12;
    unsigned int* grid = new unsigned int[(gridX + 1) * (gridZ + 1)];
    for (int j = 0; j <= gridZ; ++j) {
        float z  = legZ0 + (legZ1 - legZ0) * float(j) / gridZ;
        float tnt = float(j) / gridZ;
        // Three-tone striping along Z: alternate turfA / turfC / turfB
        int stripe = (j / 4) % 3;
        const float* base = (stripe == 0) ? turfA : (stripe == 1) ? turfC : turfB;
        for (int i = 0; i <= gridX; ++i) {
            float x  = legXMin + (legXMax - legXMin) * float(i) / gridX;
            float y  = legHeight(z);
            // Finite-difference normal
            float dy = (legHeight(z + 0.05f) - legHeight(z - 0.05f)) / 0.1f;
            float nx = 0.0f, ny = 1.0f, nz = -dy;
            float ln = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= ln; ny /= ln; nz /= ln;
            // Edge darkening (scorched edges near rock)
            float edgeFade = 1.0f;
            float xFrac    = float(i) / gridX;
            if (xFrac < 0.15f)      edgeFade = 0.78f + xFrac / 0.15f * 0.22f;
            else if (xFrac > 0.85f) edgeFade = 0.78f + (1.0f - xFrac) / 0.15f * 0.22f;
            grid[j * (gridX + 1) + i] =
                mb.addVertex(x, y + 0.05f, z, nx, ny, nz,
                             base[0] * edgeFade,
                             base[1] * edgeFade,
                             base[2] * edgeFade);
        }
    }
    for (int j = 0; j < gridZ; ++j) {
        for (int i = 0; i < gridX; ++i) {
            unsigned int a = grid[ j      * (gridX + 1) + i    ];
            unsigned int b = grid[ j      * (gridX + 1) + i + 1];
            unsigned int c = grid[(j + 1) * (gridX + 1) + i + 1];
            unsigned int d = grid[(j + 1) * (gridX + 1) + i    ];
            mb.addQuad(a, d, c, b);
        }
    }
    delete[] grid;

    // ── Obsidian ledge strips along fairway edges ─────────────────────────────
    // These are thin raised strips that catch the light dramatically.
    const int LEDGE_SEGS = 12;
    for (int seg = 0; seg < LEDGE_SEGS; ++seg) {
        float z0 = legZ0 + (legZ1 - legZ0) * float(seg)     / LEDGE_SEGS;
        float z1 = legZ0 + (legZ1 - legZ0) * float(seg + 1) / LEDGE_SEGS;
        float y0 = legHeight(z0);
        float y1 = legHeight(z1);
        // Left ledge
        addFlatQuad(mb,
                    legXMin - 0.35f, y0 + 0.06f, z0,
                    legXMin - 0.35f, y1 + 0.06f, z1,
                    legXMin + 0.10f, y1 + 0.06f, z1,
                    legXMin + 0.10f, y0 + 0.06f, z0,
                    obsidian[0], obsidian[1], obsidian[2]);
        // Right ledge
        addFlatQuad(mb,
                    legXMax - 0.10f, y0 + 0.06f, z0,
                    legXMax - 0.10f, y1 + 0.06f, z1,
                    legXMax + 0.35f, y1 + 0.06f, z1,
                    legXMax + 0.35f, y0 + 0.06f, z0,
                    obsidian[0], obsidian[1], obsidian[2]);
    }

    // ── Pumice / ash patches on the fairway itself ────────────────────────────
    // Scattered light-coloured patches baked into the geometry.
    struct PumicePatch { float x, z, rx, rz; };
    PumicePatch pp[] = {
        { -4.5f, 12.0f, 1.2f, 0.7f },
        {  3.8f, 22.0f, 0.9f, 0.6f },
        { -2.0f, 33.0f, 1.0f, 0.8f },
        {  4.5f, 44.0f, 0.8f, 0.5f },
    };
    for (int i = 0; i < 4; ++i) {
        float z = pp[i].z;
        float y = legHeight(z) + 0.06f;
        // Approximate with a flat box (thin slab)
        addBox(mb, pp[i].x - pp[i].rx, y, z - pp[i].rz,
               pp[i].rx * 2.0f, 0.02f, pp[i].rz * 2.0f,
               pumice[0], pumice[1], pumice[2]);
    }

    // ── Dog-leg bend pad ──────────────────────────────────────────────────────
    // Subdivided 6×4 for stripe pattern
    const int bendDivX = 8, bendDivZ = 6;
    for (int pj = 0; pj < bendDivZ; ++pj) {
        for (int pi = 0; pi < bendDivX; ++pi) {
            float x0 = bendX0 + (bendX1 - bendX0) * float(pi)     / bendDivX;
            float x1 = bendX0 + (bendX1 - bendX0) * float(pi + 1) / bendDivX;
            float z0 = bendZ0 + (bendZ1 - bendZ0) * float(pj)     / bendDivZ;
            float z1 = bendZ0 + (bendZ1 - bendZ0) * float(pj + 1) / bendDivZ;
            // Diagonal stripe
            float diag = float((pi + pj) % 2) * 0.04f;
            addFlatQuad(mb,
                        x0, legYTop + 0.05f, z0,
                        x0, legYTop + 0.05f, z1,
                        x1, legYTop + 0.05f, z1,
                        x1, legYTop + 0.05f, z0,
                        turfB[0] - diag, turfB[1] - diag, turfB[2] - diag);
        }
    }

    // Side walls of elevated bend (thicker, with obsidian cap strip)
    // Far wall (z = bendZ1)
    addFlatQuad(mb,
                bendX0, 0.0f,    bendZ1,
                bendX1, 0.0f,    bendZ1,
                bendX1, legYTop, bendZ1,
                bendX0, legYTop, bendZ1,
                rockC[0]*0.7f, rockC[1]*0.7f, rockC[2]*0.7f);
    addFlatQuad(mb, // obsidian cap
                bendX0, legYTop, bendZ1,
                bendX1, legYTop, bendZ1,
                bendX1, legYTop + 0.12f, bendZ1,
                bendX0, legYTop + 0.12f, bendZ1,
                obsidian[0], obsidian[1], obsidian[2]);
    // Right wall (x = bendX1)
    addFlatQuad(mb,
                bendX1, 0.0f,    bendZ0,
                bendX1, 0.0f,    bendZ1,
                bendX1, legYTop, bendZ1,
                bendX1, legYTop, bendZ0,
                rockC[0]*0.75f, rockC[1]*0.75f, rockC[2]*0.75f);

    // ── Lava pool / caldera on the bend ──────────────────────────────────────
    // Position it prominently near the back-right of the bend.
    _lavaPoolCenter = glm::vec3(30.0f, legYTop + 0.10f, 66.0f);
    // The lava mesh is built separately (see below after uploadMesh).
    // We add a cooled-lava surround ring in the main mesh:
    addRing(mb,
            _lavaPoolCenter.x, _lavaPoolCenter.y - 0.01f, _lavaPoolCenter.z,
            4.5f, 7.0f, 40,
            darkLava[0] * 1.4f, darkLava[1] * 1.4f, darkLava[2] * 1.4f);
    // Cracked obsidian rim
    addRing(mb,
            _lavaPoolCenter.x, _lavaPoolCenter.y + 0.02f, _lavaPoolCenter.z,
            6.8f, 7.4f, 40,
            obsidian[0], obsidian[1], obsidian[2]);

    // ── Rock formations (border walls) ────────────────────────────────────────
    // More varied colours per formation.
    addRockFormation(mb,
                     legXMin - 0.6f, legZ0 - 0.8f,
                     legXMin - 0.6f, legZ1 + 0.5f,
                     0.0f, 2.4f,
                     rockC[0], rockC[1], rockC[2], 11u);
    addRockFormation(mb,
                     legXMax + 0.6f, legZ0 - 0.8f,
                     legXMax + 0.6f, legZ1 + 0.5f,
                     0.0f, 2.4f,
                     rockC[0]*1.08f, rockC[1]*1.05f, rockC[2]*1.05f, 22u);
    // Tee back wall (slightly lighter — facing the camera)
    addRockFormation(mb,
                     legXMin - 0.6f, legZ0 - 0.8f,
                     legXMax + 0.6f, legZ0 - 0.8f,
                     0.0f, 2.2f,
                     rockC[0]*0.92f, rockC[1]*0.90f, rockC[2]*0.90f, 33u);
    // Bend pad borders
    addRockFormation(mb,
                     bendX0 - 0.5f, bendZ0,
                     bendX0 - 0.5f, bendZ1 + 0.5f,
                     legYTop, 2.0f,
                     rockC[0], rockC[1]*1.02f, rockC[2], 44u);
    addRockFormation(mb,
                     bendX0 - 0.5f, bendZ1 + 0.5f,
                     bendX1 + 0.5f, bendZ1 + 0.5f,
                     legYTop, 2.0f,
                     rockC[0]*0.95f, rockC[1]*0.95f, rockC[2]*1.05f, 55u);
    addRockFormation(mb,
                     bendX1 + 0.5f, bendZ0,
                     bendX1 + 0.5f, bendZ1 + 0.5f,
                     legYTop, 2.0f,
                     rockC[0]*1.08f, rockC[1]*1.06f, rockC[2]*1.06f, 66u);
    // Inner stub
    addRockFormation(mb,
                     legXMax + 0.6f, bendZ0,
                     bendX1 + 0.5f, bendZ0,
                     legYTop, 2.0f,
                     rockC[0]*0.88f, rockC[1]*0.88f, rockC[2]*0.88f, 77u);

    // ── Boulder obstacles ────────────────────────────────────────────────────
    addBoulder(mb,  2.5f, legHeight(18.0f) + 1.3f, 18.0f, 1.7f,
               rockC[0], rockC[1], rockC[2], 1337u);
    addBoulder(mb, -3.2f, legHeight(28.0f) + 1.1f, 28.0f, 1.3f,
               rockC[0]*1.12f, rockC[1]*1.08f, rockC[2]*1.08f, 2222u);
    addBoulder(mb,  4.0f, legHeight(38.0f) + 1.0f, 38.0f, 1.1f,
               rockC[0]*0.92f, rockC[1]*0.92f, rockC[2]*0.92f, 3333u);
    // Bend boulders — darker, more imposing
    addBoulder(mb,  8.0f, legYTop + 1.1f, 57.0f, 1.5f,
               rockC[0]*1.05f, rockC[1]*1.02f, rockC[2]*1.02f, 4444u);
    addBoulder(mb, 18.0f, legYTop + 0.9f, 55.0f, 1.2f,
               rockC[0], rockC[1], rockC[2], 5555u);
    // A large boss boulder near the lava pool
    addBoulder(mb, 22.0f, legYTop + 1.6f, 64.0f, 2.0f,
               rockC[0]*0.85f, rockC[1]*0.82f, rockC[2]*0.80f, 6666u);
    addBoulder(mb, 36.0f, legYTop + 1.2f, 72.0f, 1.6f,
               rockC[0]*1.10f, rockC[1]*1.08f, rockC[2]*1.05f, 7777u);

    // Pumice boulders (light colour — contrast against dark basalt)
    addBoulder(mb, -2.0f, legHeight(8.0f) + 0.7f, 8.0f, 0.9f,
               pumice[0], pumice[1], pumice[2], 8881u);
    addBoulder(mb,  5.5f, legHeight(46.0f) + 0.8f, 46.0f, 0.8f,
               pumice[0]*0.92f, pumice[1]*0.92f, pumice[2]*0.92f, 8882u);

    // ── Bone obstacle ────────────────────────────────────────────────────────
    addBoneObstacle(mb,
                    16.0f, legYTop + 0.05f, 60.0f,
                    7.0f, 0.85f, 0.38f,
                    boneC[0], boneC[1], boneC[2]);

    // ── Concrete tee pad ─────────────────────────────────────────────────────
    addBox(mb, -2.5f, 0.0f, -4.0f, 5.0f, 0.28f, 3.5f,
           concrete[0], concrete[1], concrete[2]);
    // Tee marker lines (darker inlay strips)
    addBox(mb, -2.4f, 0.28f, -3.8f, 0.25f, 0.02f, 3.0f,
           0.40f, 0.38f, 0.36f);
    addBox(mb,  2.15f, 0.28f, -3.8f, 0.25f, 0.02f, 3.0f,
           0.40f, 0.38f, 0.36f);

    // ── Cup + flag ───────────────────────────────────────────────────────────
    _cupPos = glm::vec3(34.0f, legYTop + 0.09f, 68.0f);
    // White sand ring around cup
    addDisc(mb, _cupPos.x, _cupPos.y + 0.01f, _cupPos.z, 1.1f, 24,
            0.88f, 0.85f, 0.78f);
    addDisc(mb, _cupPos.x, _cupPos.y + 0.02f, _cupPos.z, 0.42f, 24,
            0.02f, 0.02f, 0.02f);
    addFlag(mb, _cupPos.x, _cupPos.y + 0.02f, _cupPos.z, 6.5f);

    _teePos      = glm::vec3(0.0f, 0.32f, -2.0f);
    _sceneCenter = glm::vec3(16.0f, 3.0f, 37.0f);
    _sceneRadius = 58.0f;

    _indexCount = uploadMesh(mb, _vao, _vbo, _ebo);
    std::cout << "[Hole13] procedural mesh: " << mb.vCount << " verts, "
              << mb.iCount << " indices\n";

    // ── Lava pool mesh (separate, emissive) ───────────────────────────────────
    buildLavaPool(_lavaPoolCenter.x, _lavaPoolCenter.y, _lavaPoolCenter.z,
                  4.0f, 4.5f, 48,
                  _lavaVao, _lavaVbo, _lavaEbo, _lavaIndexCount);
    std::cout << "[Hole13] lava pool mesh: " << _lavaIndexCount << " indices\n";
}

// ── Internal draw helpers ─────────────────────────────────────────────────────
void Hole13::drawProcedural(const glm::mat4& mvp,
                             const glm::mat4& model,
                             const glm::mat3& nrm,
                             const glm::vec3& lightDir,
                             const glm::vec3& viewPos) const
{
    // Set all standard + new uniforms
    if (_uMVP        >= 0) glUniformMatrix4fv(_uMVP,       1, GL_FALSE, glm::value_ptr(mvp));
    if (_uModel      >= 0) glUniformMatrix4fv(_uModel,     1, GL_FALSE, glm::value_ptr(model));
    if (_uNormalMat  >= 0) glUniformMatrix3fv(_uNormalMat, 1, GL_FALSE, glm::value_ptr(nrm));
    if (_uLightDir   >= 0) glUniform3fv(_uLightDir,   1, glm::value_ptr(lightDir));
    if (_uLightColor >= 0) glUniform3f(_uLightColor,  1.00f, 0.96f, 0.88f);
    if (_uAmbient    >= 0) glUniform3f(_uAmbient,     0.25f, 0.24f, 0.28f);
    if (_uViewPos    >= 0) glUniform3fv(_uViewPos,    1, glm::value_ptr(viewPos));

    // Lava light (warm point light coming from the pool)
    if (_uLavaLightPos       >= 0) glUniform3fv(_uLavaLightPos, 1,
                                       glm::value_ptr(_lavaPoolCenter));
    if (_uLavaLightColor     >= 0) glUniform3f(_uLavaLightColor,     1.0f, 0.45f, 0.05f);
    if (_uLavaLightIntensity >= 0) glUniform1f(_uLavaLightIntensity, 2.5f);

    // Not emissive for the main mesh
    if (_uEmissive      >= 0) glUniform1f(_uEmissive, 0.0f);
    if (_uEmissiveColor >= 0) glUniform3f(_uEmissiveColor, 1.0f, 0.35f, 0.0f);

    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ── draw ─────────────────────────────────────────────────────────────────────
void Hole13::draw(const glm::mat4& view,
                  const glm::mat4& proj,
                  const glm::vec3& lightDir,
                  const glm::vec3& viewPos) const
{
    if (_program == 0 || _vao == 0) return;
    glUseProgram(_program);

    glm::mat4 model(1.0f);
    glm::mat4 mvp = proj * view * model;
    glm::mat3 nrm(1.0f);

    // ── Main procedural mesh ─────────────────────────────────────────────────
    drawProcedural(mvp, model, nrm, lightDir, viewPos);

    // ── Lava pool (emissive) ─────────────────────────────────────────────────
    if (_lavaVao) {
        if (_uMVP        >= 0) glUniformMatrix4fv(_uMVP,       1, GL_FALSE, glm::value_ptr(mvp));
        if (_uModel      >= 0) glUniformMatrix4fv(_uModel,     1, GL_FALSE, glm::value_ptr(model));
        if (_uNormalMat  >= 0) glUniformMatrix3fv(_uNormalMat, 1, GL_FALSE, glm::value_ptr(nrm));
        if (_uViewPos    >= 0) glUniform3fv(_uViewPos, 1, glm::value_ptr(viewPos));
        // Flag as emissive — the fragment shader will use the self-glow path
        if (_uEmissive      >= 0) glUniform1f(_uEmissive,      1.0f);
        if (_uEmissiveColor >= 0) glUniform3f(_uEmissiveColor, 1.0f, 0.50f, 0.05f);
        glBindVertexArray(_lavaVao);
        glDrawElements(GL_TRIANGLES, _lavaIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // ── Volcano GLB asset ────────────────────────────────────────────────────
    drawVolcano(view, proj, lightDir, viewPos);
}

void Hole13::drawVolcano(const glm::mat4& view,
                          const glm::mat4& proj,
                          const glm::vec3& lightDir,
                          const glm::vec3& viewPos) const
{
    if (!_volcano) return;

    // Place the volcano behind the bend pad, scaled and centred.
    // The GLB bounding box is roughly -9.5..9.5 X, 0..20 Y, -9.7..9.1 Z.
    // We scale up ~2× and offset to sit behind the lava pool.
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.4f, 2.4f, 2.4f));
    glm::mat4 trans = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(_lavaPoolCenter.x,
                                               -0.05f,
                                               _lavaPoolCenter.z + 4.0f));
    glm::mat4 model = trans * scale;
    glm::mat4 mvp   = proj * view * model;
    // Normal matrix: transpose of inverse of upper-left 3×3
    // (uniform scale so it equals the upper-left 3×3 itself)
    glm::mat3 nrm   = glm::mat3(model);

    if (_uMVP        >= 0) glUniformMatrix4fv(_uMVP,       1, GL_FALSE, glm::value_ptr(mvp));
    if (_uModel      >= 0) glUniformMatrix4fv(_uModel,     1, GL_FALSE, glm::value_ptr(model));
    if (_uNormalMat  >= 0) glUniformMatrix3fv(_uNormalMat, 1, GL_FALSE, glm::value_ptr(nrm));
    if (_uLightDir   >= 0) glUniform3fv(_uLightDir,   1, glm::value_ptr(lightDir));
    if (_uLightColor >= 0) glUniform3f(_uLightColor,  1.00f, 0.96f, 0.88f);
    if (_uAmbient    >= 0) glUniform3f(_uAmbient,     0.22f, 0.22f, 0.26f);
    if (_uViewPos    >= 0) glUniform3fv(_uViewPos,    1, glm::value_ptr(viewPos));
    if (_uLavaLightPos       >= 0) glUniform3fv(_uLavaLightPos, 1,
                                       glm::value_ptr(_lavaPoolCenter));
    if (_uLavaLightColor     >= 0) glUniform3f(_uLavaLightColor,     1.0f, 0.45f, 0.05f);
    if (_uLavaLightIntensity >= 0) glUniform1f(_uLavaLightIntensity, 3.0f);

    for (int i = 0; i < _volcano->primCount; ++i) {
        gltfloader::GltfPrimitive& prim = _volcano->prims[i];

        if (prim.isEmissive) {
            if (_uEmissive      >= 0) glUniform1f(_uEmissive, 1.0f);
            if (_uEmissiveColor >= 0) glUniform3f(_uEmissiveColor,
                                                  prim.emissiveR,
                                                  prim.emissiveG,
                                                  prim.emissiveB);
        } else {
            if (_uEmissive      >= 0) glUniform1f(_uEmissive, 0.0f);
            if (_uEmissiveColor >= 0) glUniform3f(_uEmissiveColor, 0.0f, 0.0f, 0.0f);
        }

        glBindVertexArray(prim.vao);
        glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

// ── cleanup ───────────────────────────────────────────────────────────────────
void Hole13::cleanup()
{
    if (_ebo)     { glDeleteBuffers(1, &_ebo);      _ebo     = 0; }
    if (_vbo)     { glDeleteBuffers(1, &_vbo);      _vbo     = 0; }
    if (_vao)     { glDeleteVertexArrays(1, &_vao); _vao     = 0; }
    if (_lavaEbo) { glDeleteBuffers(1, &_lavaEbo);  _lavaEbo = 0; }
    if (_lavaVbo) { glDeleteBuffers(1, &_lavaVbo);  _lavaVbo = 0; }
    if (_lavaVao) { glDeleteVertexArrays(1, &_lavaVao); _lavaVao = 0; }
    gltfloader::freeGltfMesh(_volcano); _volcano = 0;
    _indexCount = _lavaIndexCount = 0;
}