#pragma once

#include "objects/Terrain.h"

class TerrainExample
{
private:
    Terrain flat;
    Terrain hills;
    Terrain rocky;

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

        f.r = 0.2f; f.g = 0.5f; f.b = 0.2f;
        flat.init(f);

        // ========================================================
        // HILLS
        // ========================================================
        hills.worldOffset = glm::vec3(-60.0f, 0.0f, 0.0f);

        TerrainConfig h{};
        h.tileWidth = 28.0f;
        h.tileLength = 28.0f;
        h.tilesX = 2;
        h.tilesZ = 2;

        h.ampA = 0.7f;
        h.ampB = 0.4f;
        h.freqA = 1.6f;
        h.freqB = 2.3f;

        h.treeDensity = 0.015f;
        h.bushDensity = 0.03f;

        hills.init(h);

        // ========================================================
        // ROCKY HEIGHTMAP TERRAIN
        // ========================================================
        rocky.worldOffset = glm::vec3(0.0f, 0.0f, 80.0f);

        TerrainConfig r{};
        r.tileWidth = 28.0f;
        r.tileLength = 28.0f;
        r.tilesX = 2;
        r.tilesZ = 2;

        r.segsX = 24;
        r.segsZ = 24;

        r.heightFn = [](float nx, float nz)
        {
            return sinf(nx * 10.0f) * cosf(nz * 8.0f) * 0.8f;
        };

        r.pebbleDensity = 0.08f;
        r.stoneField = false;

        rocky.init(r);
    }

    void draw(GLuint shader, bool wireframe)
    {
        flat.draw(shader, wireframe);
        hills.draw(shader, wireframe);
        rocky.draw(shader, wireframe);
    }
};