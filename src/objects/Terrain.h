#pragma once

// ============================================================
// Allowed headers only (COS344)
// ============================================================
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <functional>

#include "../core/Mesh.h"

// ============================================================
// Forward declarations
// ============================================================
class Terrain;

// ============================================================
// TerrainConfig — everything about how a Terrain tile looks.
//
// Build one, tweak its fields, pass it to Terrain::init().
// Sane defaults produce a flat grassy 1×1 tile.
// ============================================================
struct TerrainConfig {

    // ── Tile dimensions ──────────────────────────────────────
    float tileWidth  = 28.0f;   // X extent of one tile (world units)
    float tileLength = 28.0f;   // Z extent of one tile

    // ── Grid — N×M tiles placed side-by-side ─────────────────
    // Total surface = (tilesX * tileWidth) × (tilesZ * tileLength)
    int   tilesX     = 1;
    int   tilesZ     = 1;

    // ── Mesh resolution per tile ──────────────────────────────
    int   segsX      = 24;      // vertex subdivisions along X
    int   segsZ      = 24;      // vertex subdivisions along Z

    // ── Base colour (grass / dirt / rock) ────────────────────
    float r = 0.18f, g = 0.42f, b = 0.14f, a = 1.0f;

    // ── Hills / terrain undulation ────────────────────────────
    // The height at each vertex is:
    //   h = ampA*sin(freqA*2π*nx + phaseA)
    //     + ampB*sin(freqB*2π*nz + phaseB)
    //     + ampC*sin(freqC*2π*(nx+nz))
    //   where nx,nz are normalised [0,1] coords over the FULL grid.
    //
    // Set all amps to 0 for a perfectly flat tile.
    float ampA   = 0.0f,  freqA = 1.5f, phaseA = 0.0f;
    float ampB   = 0.0f,  freqB = 2.3f, phaseB = 0.8f;
    float ampC   = 0.0f,  freqC = 1.0f;

    // Optional custom height function — overrides the sinusoid
    // model if non-null.  Receives (nx, nz) both in [0,1] over
    // the full grid (i.e. across all tiles combined).
    std::function<float(float,float)> heightFn = nullptr;

    // ── Foliage density ───────────────────────────────────────
    // Trees, bushes, pebbles are placed on a jittered grid.
    // density = average objects per unit area.
    // 0 = none, 0.02 = sparse, 0.1 = dense.
    float treeDensity   = 0.0f;
    float bushDensity   = 0.0f;
    float pebbleDensity = 0.0f;

    // Scale multipliers for foliage objects
    float treeScale    = 1.0f;
    float bushScale    = 1.0f;
    float pebbleScale  = 0.5f;

    // Foliage placement: objects are placed on the terrain surface
    // (their Y = terrain height at their XZ). Set to false to pin
    // foliage to Y=0 (useful for pre-flattened tiles).
    bool foliageOnSurface = true;

    // ── Kerb / border ────────────────────────────────────────
    // Raised concrete border around the perimeter of the whole grid.
    bool  showBorder  = false;
    float borderH     = 0.25f;
    float borderW     = 0.35f;
    float borderR     = 0.72f, borderG = 0.70f, borderB = 0.66f;

    // ── Pebble stone field (small scattered rocks ON the surface) ─
    // Separate from the rock-obstacle pebbles — these are tiny,
    // cosmetic, and very numerous.
    bool  stoneField  = false;

    // ── Random seed ───────────────────────────────────────────
    int   seed        = 42;
};

// ============================================================
// Foliage — static factory classes placeable anywhere.
//
// Each static method pushes ready-to-draw Mesh* pointers into
// an external array so they can live in a scene list alongside
// hole meshes.  Caller owns the pointers.
//
// All methods call mesh->setup() before returning.
// ============================================================
class Foliage {
public:

    // ── Tree ─────────────────────────────────────────────────
    // A trunk cylinder + two stacked cone layers.
    // wx,wz : world position (Y is placed at baseY).
    // scale : overall scale multiplier (default 1 = ~6 units tall).
    // seed  : deterministic variation in colour and lean.
    static void makeTree(Mesh** out, int& count, int maxOut,
                         float wx, float baseY, float wz,
                         float scale = 1.0f, int seed = 0);

    // ── Bush ─────────────────────────────────────────────────
    // A flattened hemisphere approximated by a wide short cone
    // over a very short cylinder stem.
    static void makeBush(Mesh** out, int& count, int maxOut,
                         float wx, float baseY, float wz,
                         float scale = 1.0f, int seed = 0);

    // ── Pebble cluster ───────────────────────────────────────
    // 3-5 small makeRock() instances grouped together, each with
    // a slight random offset and rotation.
    static void makePebbleCluster(Mesh** out, int& count, int maxOut,
                                  float wx, float baseY, float wz,
                                  float scale = 0.5f, int seed = 0);

    // ── Fence post ───────────────────────────────────────────
    // Thin cylinder, useful for bridge railings or boundaries.
    static void makeFencePost(Mesh** out, int& count, int maxOut,
                              float wx, float baseY, float wz,
                              float height = 1.2f, float radius = 0.08f);

    // ── Dead tree / stump ────────────────────────────────────
    // Short cylinder (stump) with two broken-off branch stubs.
    static void makeStump(Mesh** out, int& count, int maxOut,
                          float wx, float baseY, float wz,
                          float scale = 1.0f, int seed = 0);

    // Deterministic noise helper — same algorithm used in Mesh.cpp
    static float rng(int seed);
    // Remap rng output to [lo, hi]
    static float rngRange(int seed, float lo, float hi);
};

// ============================================================
// Terrain — N×M tiled ground surface with optional foliage.
//
// Usage:
//   TerrainConfig cfg;
//   cfg.tileWidth  = 28.0f;
//   cfg.tileLength = 28.0f;
//   cfg.tilesX     = 3;
//   cfg.tilesZ     = 2;
//   cfg.ampA       = 0.6f;   // gentle hills
//   cfg.treeDensity= 0.02f;
//
//   Terrain terrain;
//   terrain.init(cfg);
//
//   // in render loop:
//   terrain.draw(shader, wireframe);
//
// The Terrain object owns all its Mesh* pointers and cleans up
// in the destructor — truly insert-and-forget.
// ============================================================
class Terrain {
public:
    Terrain();
    ~Terrain();

    // Build the terrain from config.  Call once after GL init.
    // Returns false and prints a message if something goes wrong.
    bool init(const TerrainConfig& cfg);

    // Draw all terrain and foliage meshes.
    void draw(GLuint shaderProgram, bool wireframe = false);

    // Apply a world-space transform to every mesh (e.g. to position
    // the terrain block next to a hole).  Call BEFORE init() or the
    // geometry will be in the wrong place.  Alternatively set
    // worldOffset before calling init().
    glm::vec3 worldOffset = glm::vec3(0.0f);

    // Expose mesh count for debugging
    int meshCount() const { return m_meshCount; }

    static const int MAX_TERRAIN_MESHES = 8192;

    Mesh* m_meshes[MAX_TERRAIN_MESHES];
    int   m_meshCount = 0;

    void addMesh(Mesh* m);

    // ── Internal builders ─────────────────────────────────────
    void buildSurface(const TerrainConfig& cfg);
    void buildFoliage(const TerrainConfig& cfg);
    void buildBorder (const TerrainConfig& cfg);

    // Evaluate the height function for a given (nx,nz) in [0,1]².
    float evalHeight(const TerrainConfig& cfg, float nx, float nz) const;

    // Deterministic placement grid with per-cell jitter.
    // Yields candidate (wx, wz) positions inside the total grid extents.
    // Returns the number of positions written into outX/outZ.
    int jitteredGrid(const TerrainConfig& cfg,
                     float density,
                     float* outX, float* outZ,
                     int maxOut,
                     int seedOffset) const;

    // Per-tile surface mesh
    void buildTile(const TerrainConfig& cfg, int tx, int tz);
};
