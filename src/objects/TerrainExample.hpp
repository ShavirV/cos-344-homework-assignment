#pragma once

#include "objects/Terrain.h"
#include <glm/glm.hpp>

class TerrainExample
{
private:
    Terrain flat;
    Terrain hills;
    Terrain rocky;
    Terrain mountains;
    Terrain canyon;
    Terrain forest;
    Terrain dunes;
    Terrain plateau;

public:

    void init()
    {
        // ========================================================
        // FLAT PATCH
        // ========================================================
        flat.worldOffset = glm::vec3(40.0f, 0.0f, 0.0f);

        TerrainConfig f{};
        f.tileWidth = 28.0f;
        f.tileLength = 28.0f;
        f.tilesX = 1;
        f.tilesZ = 1;
        f.segsX = 16;
        f.segsZ = 16;

        f.r = 0.20f; f.g = 0.55f; f.b = 0.22f;

        flat.init(f);

        // ========================================================
        // SIMPLE HILLS
        // ========================================================
        hills.worldOffset = glm::vec3(-60.0f, 0.0f, 0.0f);

        TerrainConfig h{};
        h.tileWidth = 28.0f;
        h.tileLength = 28.0f;
        h.tilesX = 2;
        h.tilesZ = 2;
        h.segsX = 24;
        h.segsZ = 24;

        h.ampA = 1.2f;
        h.freqA = 1.3f;

        h.ampB = 0.6f;
        h.freqB = 2.4f;

        h.treeDensity = 0.010f;
        h.bushDensity = 0.020f;
        h.pebbleDensity = 0.010f;

        hills.init(h);

        // ========================================================
        // ROCKY TERRAIN (sharp variation, but small scale)
        // ========================================================
        rocky.worldOffset = glm::vec3(0.0f, 0.0f, 80.0f);

        TerrainConfig r{};
        r.tileWidth = 28.0f;
        r.tileLength = 28.0f;
        r.tilesX = 2;
        r.tilesZ = 2;
        r.segsX = 32;
        r.segsZ = 32;

        r.heightFn = [](float nx, float nz)
        {
            float base  = sinf(nx * 8.0f) * cosf(nz * 7.0f);
            float sharp = fabsf(sinf((nx + nz) * 6.0f)) * 0.8f;
            return base * 0.8f + sharp;
        };

        r.pebbleDensity = 0.08f;
        r.treeDensity = 0.005f;
        r.stoneField = false;

        rocky.init(r);

        // ========================================================
        // MOUNTAINS (REALISTIC LARGE VARIANCE)
        // FIX: scale domain, not just amplitude
        // ========================================================
        mountains.worldOffset = glm::vec3(0.0f, 0.0f, -60.0f);

        TerrainConfig m{};
        m.tileWidth = 28.0f;
        m.tileLength = 28.0f;
        m.tilesX = 3;
        m.tilesZ = 3;
        m.segsX = 32;
        m.segsZ = 32;

        // IMPORTANT: moderate amps, but strong spatial scaling
        m.ampA = 3.0f;
        m.freqA = 0.6f;

        m.ampB = 2.2f;
        m.freqB = 1.1f;

        m.ampC = 1.5f;
        m.freqC = 2.2f;

        m.heightFn = [](float x, float z)
        {
            // large mountain ridges
            float ridge1 = sinf(x * 0.8f) * 4.0f;
            float ridge2 = cosf(z * 0.7f) * 4.0f;

            // secondary peaks
            float peaks = fabsf(sinf((x + z) * 0.9f)) * 6.0f;

            // erosion noise
            float noise = sinf(x * 2.5f) * sinf(z * 2.0f) * 0.8f;

            return ridge1 + ridge2 + peaks + noise;
        };

        m.treeDensity = 0.006f;
        m.bushDensity = 0.010f;
        m.pebbleDensity = 0.020f;
        m.stoneField = false;

        mountains.init(m);

        // ========================================================
        // CANYON (compressed Z, readable structure)
        // ========================================================
        canyon.worldOffset = glm::vec3(-140.0f, 0.0f, -40.0f);

        TerrainConfig c{};
        c.tileWidth = 56.0f;
        c.tileLength = 28.0f;
        c.tilesX = 2;
        c.tilesZ = 1;
        c.segsX = 32;
        c.segsZ = 24;

        c.heightFn = [](float x, float z)
        {
            float walls = fabsf(sinf(x * 1.8f)) * 3.0f;
            float floor = sinf(z * 2.0f) * 0.5f;
            return walls + floor;
        };

        c.treeDensity = 0.001f;
        c.bushDensity = 0.008f;
        c.pebbleDensity = 0.03f;

        canyon.init(c);

        // ========================================================
        // FOREST (dense but small)
        // ========================================================
        forest.worldOffset = glm::vec3(0.0f, 0.0f, -140.0f);

        TerrainConfig fo{};
        fo.tileWidth = 28.0f;
        fo.tileLength = 28.0f;
        fo.tilesX = 3;
        fo.tilesZ = 3;
        fo.segsX = 24;
        fo.segsZ = 24;

        fo.ampA = 0.9f;
        fo.ampB = 0.4f;

        fo.treeDensity = 0.035f;
        fo.bushDensity = 0.060f;
        fo.pebbleDensity = 0.020f;
        fo.stoneField = false;

        forest.init(fo);

        // ========================================================
        // DESERT DUNES
        // ========================================================
        dunes.worldOffset = glm::vec3(180.0f, 0.0f, 120.0f);

        TerrainConfig d{};
        d.tileWidth = 40.0f;
        d.tileLength = 40.0f;
        d.tilesX = 2;
        d.tilesZ = 2;
        d.segsX = 32;
        d.segsZ = 32;

        d.heightFn = [](float x, float z)
        {
            return sinf(x * 1.2f) * 1.5f + cosf(z * 1.0f) * 1.2f;
        };

        d.treeDensity = 0.0f;
        d.bushDensity = 0.0f;
        d.pebbleDensity = 0.015f;

        d.r = 0.76f;
        d.g = 0.70f;
        d.b = 0.45f;

        dunes.init(d);

        // ========================================================
        // PLATEAU (stable flat top)
        // ========================================================
        plateau.worldOffset = glm::vec3(-180.0f, 0.0f, 120.0f);

        TerrainConfig p{};
        p.tileWidth = 40.0f;
        p.tileLength = 40.0f;
        p.tilesX = 2;
        p.tilesZ = 2;
        p.segsX = 24;
        p.segsZ = 24;

        p.heightFn = [](float x, float z)
        {
            float edge = fabsf(x * z) * 0.05f;
            return 2.5f - edge;
        };

        p.treeDensity = 0.01f;
        p.bushDensity = 0.02f;
        p.stoneField = false;

        plateau.init(p);
    }

    void draw(GLuint shader, bool wireframe)
    {
        flat.draw(shader, wireframe);
        hills.draw(shader, wireframe);
        rocky.draw(shader, wireframe);
        mountains.draw(shader, wireframe);
        canyon.draw(shader, wireframe);
        forest.draw(shader, wireframe);
        dunes.draw(shader, wireframe);
        plateau.draw(shader, wireframe);
    }
};