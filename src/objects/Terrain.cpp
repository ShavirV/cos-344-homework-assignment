#include "Terrain.h"
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ============================================================
// Foliage helpers
// ============================================================

float Foliage::rng(int seed)
{
    // Identical hash to Mesh.cpp's rockRand — keeps determinism
    // consistent if the same seed is used across both systems.
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9301;
    seed ^= seed << 4;
    seed *= 0x27d4eb2d;
    seed ^= seed >> 15;
    return (float)(seed & 0xFFFF) / 32767.5f - 1.0f;  // [-1, 1]
}

float Foliage::rngRange(int seed, float lo, float hi)
{
    return lo + (rng(seed) * 0.5f + 0.5f) * (hi - lo);  // [lo, hi]
}

// ── Foliage::makeTree ────────────────────────────────────────────────────────
void Foliage::makeTree(Mesh** out, int& count, int maxOut,
                       float wx, float baseY, float wz,
                       float scale, int seed)
{
    if (count + 3 > maxOut) return;

    // Slight colour variation: darker or lighter green
    float gv = rngRange(seed * 7 + 1, 0.38f, 0.55f);
    float bv = rngRange(seed * 7 + 2, 0.08f, 0.14f);

    float trunkH  = rngRange(seed + 10, 3.0f, 5.0f) * scale;
    float trunkR  = rngRange(seed + 11, 0.22f, 0.35f) * scale;
    float cone1R  = rngRange(seed + 12, 1.8f, 2.8f) * scale;
    float cone1H  = rngRange(seed + 13, 2.5f, 3.5f) * scale;
    float cone2R  = cone1R * 0.65f;
    float cone2H  = cone1H * 0.85f;

    // Trunk
    Mesh* trunk = new Mesh(Mesh::makeCylinder(
        trunkR, trunkH, 10,
        0.36f, 0.24f, 0.12f, 1.0f));
    trunk->translate(wx, baseY + trunkH * 0.5f, wz);
    trunk->setup();
    out[count++] = trunk;

    // Lower canopy cone
    Mesh* low = new Mesh(Mesh::makeCone(
        cone1R, cone1H, 14,
        0.10f, gv, bv, 1.0f));
    low->translate(wx, baseY + trunkH, wz);
    low->setup();
    out[count++] = low;

    // Upper canopy cone (narrower, offset up)
    Mesh* top = new Mesh(Mesh::makeCone(
        cone2R, cone2H, 12,
        0.08f, gv * 1.1f, bv * 1.05f, 1.0f));
    top->translate(wx, baseY + trunkH + cone1H * 0.55f, wz);
    top->setup();
    out[count++] = top;
}

// ── Foliage::makeBush ────────────────────────────────────────────────────────
void Foliage::makeBush(Mesh** out, int& count, int maxOut,
                       float wx, float baseY, float wz,
                       float scale, int seed)
{
    if (count + 2 > maxOut) return;

    float gv    = rngRange(seed * 5 + 3, 0.40f, 0.60f);
    float stemH = rngRange(seed + 20, 0.15f, 0.30f) * scale;
    float stemR = 0.10f * scale;
    float bushR = rngRange(seed + 21, 0.9f, 1.6f) * scale;
    float bushH = rngRange(seed + 22, 0.6f, 1.1f) * scale;

    // Stem
    Mesh* stem = new Mesh(Mesh::makeCylinder(
        stemR, stemH, 6,
        0.30f, 0.20f, 0.10f, 1.0f));
    stem->translate(wx, baseY + stemH * 0.5f, wz);
    stem->setup();
    out[count++] = stem;

    // Bush canopy — wide, flat cone (low height-to-radius ratio)
    Mesh* canopy = new Mesh(Mesh::makeCone(
        bushR, bushH, 12,
        0.12f, gv, 0.10f, 1.0f));
    canopy->translate(wx, baseY + stemH, wz);
    canopy->setup();
    out[count++] = canopy;
}

// ── Foliage::makePebbleCluster ───────────────────────────────────────────────
void Foliage::makePebbleCluster(Mesh** out, int& count, int maxOut,
                                float wx, float baseY, float wz,
                                float scale, int seed)
{
    int n = 3 + (int)(rngRange(seed * 3, 0.0f, 2.99f));  // 3–5 pebbles
    if (count + n > maxOut) return;

    for (int i = 0; i < n; i++) {
        float ox    = rngRange(seed * 13 + i * 7,  -0.5f, 0.5f) * scale;
        float oz    = rngRange(seed * 17 + i * 11, -0.5f, 0.5f) * scale;
        float ps    = rngRange(seed * 19 + i * 5,   0.08f, 0.22f) * scale;
        float rotY  = rngRange(seed * 23 + i,       0.0f, 360.0f);

        Mesh* p = new Mesh(Mesh::makeRock(ps));
        p->rotate(rotY, 0.0f, 1.0f, 0.0f);
        p->translate(wx + ox, baseY + ps * 0.3f, wz + oz);
        p->setup();
        out[count++] = p;
    }
}

// ── Foliage::makeFencePost ───────────────────────────────────────────────────
void Foliage::makeFencePost(Mesh** out, int& count, int maxOut,
                            float wx, float baseY, float wz,
                            float height, float radius)
{
    if (count + 1 > maxOut) return;

    Mesh* post = new Mesh(Mesh::makeCylinder(
        radius, height, 6,
        0.48f, 0.32f, 0.16f, 1.0f));
    post->translate(wx, baseY + height * 0.5f, wz);
    post->setup();
    out[count++] = post;
}

// ── Foliage::makeStump ───────────────────────────────────────────────────────
void Foliage::makeStump(Mesh** out, int& count, int maxOut,
                        float wx, float baseY, float wz,
                        float scale, int seed)
{
    if (count + 3 > maxOut) return;

    float stumpR = rngRange(seed + 30, 0.3f, 0.55f) * scale;
    float stumpH = rngRange(seed + 31, 0.5f, 1.0f) * scale;

    // Main stump body
    Mesh* stump = new Mesh(Mesh::makeCylinder(
        stumpR, stumpH, 8,
        0.40f, 0.28f, 0.14f, 1.0f));
    stump->translate(wx, baseY + stumpH * 0.5f, wz);
    stump->setup();
    out[count++] = stump;

    // Two broken branch stubs at random angles
    for (int bi = 0; bi < 2; bi++) {
        float bR   = stumpR * rngRange(seed + 40 + bi, 0.25f, 0.45f);
        float bH   = rngRange(seed + 50 + bi, 0.4f, 0.8f) * scale;
        float bAng = rngRange(seed + 60 + bi, 20.0f, 55.0f);  // tilt from vertical
        float bRot = rngRange(seed + 70 + bi, 0.0f, 360.0f);  // spin around Y

        Mesh* branch = new Mesh(Mesh::makeCylinder(
            bR, bH, 6,
            0.38f, 0.26f, 0.12f, 1.0f));
        // Tilt outward from trunk, then spin
        branch->rotate(bAng, 0.0f, 0.0f, 1.0f);
        branch->rotate(bRot, 0.0f, 1.0f, 0.0f);
        branch->translate(wx, baseY + stumpH * 0.85f, wz);
        branch->setup();
        out[count++] = branch;
    }
}

// ============================================================
// Terrain
// ============================================================

Terrain::Terrain()
{
    for (int i = 0; i < MAX_TERRAIN_MESHES; i++)
        m_meshes[i] = nullptr;
}

Terrain::~Terrain()
{
    for (int i = 0; i < m_meshCount; i++) {
        delete m_meshes[i];
        m_meshes[i] = nullptr;
    }
}

void Terrain::addMesh(Mesh* m)
{
    if (m_meshCount < MAX_TERRAIN_MESHES)
        m_meshes[m_meshCount++] = m;
    else
        std::cerr << "[Terrain] MAX_TERRAIN_MESHES exceeded — mesh dropped\n";
}

// ── evalHeight ───────────────────────────────────────────────────────────────
float Terrain::evalHeight(const TerrainConfig& cfg, float nx, float nz) const
{
    if (cfg.heightFn)
        return cfg.heightFn(nx, nz);

    float h = 0.0f;
    if (cfg.ampA != 0.0f)
        h += cfg.ampA * sinf(cfg.freqA * 2.0f * (float)M_PI * nx + cfg.phaseA);
    if (cfg.ampB != 0.0f)
        h += cfg.ampB * sinf(cfg.freqB * 2.0f * (float)M_PI * nz + cfg.phaseB);
    if (cfg.ampC != 0.0f)
        h += cfg.ampC * sinf(cfg.freqC * 2.0f * (float)M_PI * (nx + nz));
    return h;
}

// ── jitteredGrid ─────────────────────────────────────────────────────────────
// Places objects on a jittered regular grid inside the total terrain extents.
// Returns the number of positions written.
int Terrain::jitteredGrid(const TerrainConfig& cfg,
                          float density,
                          float* outX, float* outZ,
                          int maxOut,
                          int seedOffset) const
{
    if (density <= 0.0f || maxOut <= 0) return 0;

    float totalW = cfg.tileWidth  * cfg.tilesX;
    float totalL = cfg.tileLength * cfg.tilesZ;
    float area   = totalW * totalL;

    // Cell size: one object per cell on average, scaled by density.
    // cellArea = 1/density  →  cell = sqrt(1/density)
    float cellSize = sqrtf(1.0f / density);
    int   cellsX   = (int)(totalW / cellSize) + 1;
    int   cellsZ   = (int)(totalL / cellSize) + 1;

    int written = 0;
    for (int iz = 0; iz < cellsZ && written < maxOut; iz++) {
        for (int ix = 0; ix < cellsX && written < maxOut; ix++) {
            int   cellSeed = seedOffset + iz * 4096 + ix;

            // Jitter within the cell [0,1] of cell size
            float jx = (Foliage::rng(cellSeed * 3 + 1) * 0.5f + 0.5f);
            float jz = (Foliage::rng(cellSeed * 3 + 2) * 0.5f + 0.5f);

            // World position (centred: terrain goes from -totalW/2 to +totalW/2)
            float wx = -totalW * 0.5f + (ix + jx) * cellSize
                       + worldOffset.x;
            float wz = -totalL * 0.5f + (iz + jz) * cellSize
                       + worldOffset.z;

            // Skip if out of bounds (jitter can push past edge)
            float hw = totalW * 0.5f, hl = totalL * 0.5f;
            if (wx < worldOffset.x - hw || wx > worldOffset.x + hw) continue;
            if (wz < worldOffset.z - hl || wz > worldOffset.z + hl) continue;

            outX[written] = wx;
            outZ[written] = wz;
            written++;
        }
    }
    return written;
}

// ── buildTile ────────────────────────────────────────────────────────────────
// Builds one surface tile at grid index (tx, tz) using makeHeightmapTerrain.
void Terrain::buildTile(const TerrainConfig& cfg, int tx, int tz)
{
    float totalW = cfg.tileWidth  * cfg.tilesX;
    float totalL = cfg.tileLength * cfg.tilesZ;

    // World-space origin of this tile's corner (bottom-left = min X, min Z)
    float originX = -totalW * 0.5f + tx * cfg.tileWidth  + worldOffset.x;
    float originZ = -totalL * 0.5f + tz * cfg.tileLength + worldOffset.z;

    // Build a heightmap for just this tile, but the height function receives
    // normalised coordinates over the FULL grid so hills continue seamlessly
    // across tile boundaries.
    auto tileFn = [&](float lnx, float lnz) -> float {
        // Convert local [0,1] to global [0,1]
        float gnx = ((float)tx + lnx) / (float)cfg.tilesX;
        float gnz = ((float)tz + lnz) / (float)cfg.tilesZ;
        return evalHeight(cfg, gnx, gnz);
    };

    Mesh* tile = new Mesh(Mesh::makeHeightmapTerrain(
        cfg.tileWidth, cfg.tileLength,
        cfg.segsX, cfg.segsZ,
        tileFn,
        cfg.r, cfg.g, cfg.b, cfg.a));

    // makeHeightmapTerrain centres the mesh at the origin — translate to
    // the correct world position.
    tile->translate(
        originX + cfg.tileWidth  * 0.5f,
        worldOffset.y,
        originZ + cfg.tileLength * 0.5f);
    tile->setup();
    addMesh(tile);
}

// ── buildSurface ─────────────────────────────────────────────────────────────
void Terrain::buildSurface(const TerrainConfig& cfg)
{
    for (int tz = 0; tz < cfg.tilesZ; tz++)
        for (int tx = 0; tx < cfg.tilesX; tx++)
            buildTile(cfg, tx, tz);
}

// ── buildFoliage ─────────────────────────────────────────────────────────────
void Terrain::buildFoliage(const TerrainConfig& cfg)
{
    float totalW = cfg.tileWidth  * cfg.tilesX;
    float totalL = cfg.tileLength * cfg.tilesZ;

    // Maximum candidate positions we'll allocate on the stack.
    // For very large grids you may want to bump this, but 4096 covers
    // a 28×28×4 = 112×112 tile at density 0.1 (≈1254 objects).
    const int MAX_POSITIONS = 4096;
    float posX[MAX_POSITIONS];
    float posZ[MAX_POSITIONS];

    // ── Trees ─────────────────────────────────────────────────
    if (cfg.treeDensity > 0.0f) {
        int n = jitteredGrid(cfg, cfg.treeDensity,
                             posX, posZ, MAX_POSITIONS,
                             cfg.seed * 1000 + 1);
        for (int i = 0; i < n; i++) {
            float nx  = (posX[i] - worldOffset.x + totalW * 0.5f) / totalW;
            float nz  = (posZ[i] - worldOffset.z + totalL * 0.5f) / totalL;
            float wy  = cfg.foliageOnSurface
                        ? (evalHeight(cfg, nx, nz) + worldOffset.y)
                        : worldOffset.y;

            Foliage::makeTree(m_meshes, m_meshCount, MAX_TERRAIN_MESHES,
                              posX[i], wy, posZ[i],
                              cfg.treeScale,
                              cfg.seed + i * 7 + 100);
        }
    }

    // ── Bushes ────────────────────────────────────────────────
    if (cfg.bushDensity > 0.0f) {
        int n = jitteredGrid(cfg, cfg.bushDensity,
                             posX, posZ, MAX_POSITIONS,
                             cfg.seed * 1000 + 2);
        for (int i = 0; i < n; i++) {
            float nx  = (posX[i] - worldOffset.x + totalW * 0.5f) / totalW;
            float nz  = (posZ[i] - worldOffset.z + totalL * 0.5f) / totalL;
            float wy  = cfg.foliageOnSurface
                        ? (evalHeight(cfg, nx, nz) + worldOffset.y)
                        : worldOffset.y;

            Foliage::makeBush(m_meshes, m_meshCount, MAX_TERRAIN_MESHES,
                              posX[i], wy, posZ[i],
                              cfg.bushScale,
                              cfg.seed + i * 11 + 200);
        }
    }

    // ── Pebble clusters ───────────────────────────────────────
    if (cfg.pebbleDensity > 0.0f) {
        int n = jitteredGrid(cfg, cfg.pebbleDensity,
                             posX, posZ, MAX_POSITIONS,
                             cfg.seed * 1000 + 3);
        for (int i = 0; i < n; i++) {
            float nx  = (posX[i] - worldOffset.x + totalW * 0.5f) / totalW;
            float nz  = (posZ[i] - worldOffset.z + totalL * 0.5f) / totalL;
            float wy  = cfg.foliageOnSurface
                        ? (evalHeight(cfg, nx, nz) + worldOffset.y)
                        : worldOffset.y;

            Foliage::makePebbleCluster(m_meshes, m_meshCount, MAX_TERRAIN_MESHES,
                                       posX[i], wy, posZ[i],
                                       cfg.pebbleScale,
                                       cfg.seed + i * 13 + 300);
        }
    }

    // ── Stone field ───────────────────────────────────────────
    // Very high density cosmetic pebbles (individually tiny).
    if (cfg.stoneField) {
        int n = jitteredGrid(cfg, 0.25f,
                             posX, posZ, MAX_POSITIONS,
                             cfg.seed * 1000 + 4);
        for (int i = 0; i < n; i++) {
            float nx  = (posX[i] - worldOffset.x + totalW * 0.5f) / totalW;
            float nz  = (posZ[i] - worldOffset.z + totalL * 0.5f) / totalL;
            float wy  = cfg.foliageOnSurface
                        ? (evalHeight(cfg, nx, nz) + worldOffset.y)
                        : worldOffset.y;

            // Tiny rocks: scale 0.05–0.12
            float ps = Foliage::rngRange(cfg.seed + i * 31, 0.05f, 0.12f);
            Mesh* stone = new Mesh(Mesh::makeRock(ps));
            stone->rotate(Foliage::rngRange(cfg.seed + i * 37, 0.0f, 360.0f),
                          0.0f, 1.0f, 0.0f);
            stone->translate(posX[i], wy + ps * 0.2f, posZ[i]);
            stone->setup();
            addMesh(stone);
        }
    }
}

// ── buildBorder ──────────────────────────────────────────────────────────────
// A raised concrete border around the full perimeter of the N×M grid,
// built from four Mesh::makeCuboid slabs (N,S,E,W edges).
void Terrain::buildBorder(const TerrainConfig& cfg)
{
    float totalW = cfg.tileWidth  * cfg.tilesX;
    float totalL = cfg.tileLength * cfg.tilesZ;
    float cx     = worldOffset.x;
    float cy     = worldOffset.y + cfg.borderH * 0.5f;
    float cz     = worldOffset.z;
    float bw     = cfg.borderW;
    float br = cfg.borderR, bg = cfg.borderG, bb = cfg.borderB;

    // North edge (max Z)
    {
        Mesh* n = new Mesh(Mesh::makeCuboid(
            totalW + bw * 2.0f, cfg.borderH, bw,
            br, bg, bb, 1.0f));
        n->translate(cx, cy, cz + totalL * 0.5f + bw * 0.5f);
        n->setup();
        addMesh(n);
    }
    // South edge (min Z)
    {
        Mesh* s = new Mesh(Mesh::makeCuboid(
            totalW + bw * 2.0f, cfg.borderH, bw,
            br, bg, bb, 1.0f));
        s->translate(cx, cy, cz - totalL * 0.5f - bw * 0.5f);
        s->setup();
        addMesh(s);
    }
    // East edge (max X)
    {
        Mesh* e = new Mesh(Mesh::makeCuboid(
            bw, cfg.borderH, totalL,
            br, bg, bb, 1.0f));
        e->translate(cx + totalW * 0.5f + bw * 0.5f, cy, cz);
        e->setup();
        addMesh(e);
    }
    // West edge (min X)
    {
        Mesh* w = new Mesh(Mesh::makeCuboid(
            bw, cfg.borderH, totalL,
            br, bg, bb, 1.0f));
        w->translate(cx - totalW * 0.5f - bw * 0.5f, cy, cz);
        w->setup();
        addMesh(w);
    }
}

// ── init ─────────────────────────────────────────────────────────────────────
bool Terrain::init(const TerrainConfig& cfg)
{
    if (cfg.tilesX <= 0 || cfg.tilesZ <= 0) {
        std::cerr << "[Terrain] tilesX and tilesZ must be >= 1\n";
        return false;
    }
    if (cfg.segsX <= 0 || cfg.segsZ <= 0) {
        std::cerr << "[Terrain] segsX and segsZ must be >= 1\n";
        return false;
    }

    buildSurface(cfg);
    buildFoliage(cfg);
    if (cfg.showBorder) buildBorder(cfg);

    std::cout << "[Terrain] built " << m_meshCount
              << " meshes  (" << cfg.tilesX << "x" << cfg.tilesZ
              << " tiles, " << cfg.segsX << "x" << cfg.segsZ
              << " segs/tile)\n";
    return true;
}

// ── draw ─────────────────────────────────────────────────────────────────────
void Terrain::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < m_meshCount; i++)
        m_meshes[i]->draw(shaderProgram, wireframe);
}
