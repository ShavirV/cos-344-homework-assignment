#include "Hole3.h"

// ─────────────────────────────────────────────────────────────────────────────
// Layout (all coords local; origin = centre of the 28×15 bounding box)
//
//   X axis: -14 (tee end) → +14 (cup end)
//   Z axis: -7.5 → +7.5
//
//   • Tee mat            : x ∈ [-14, -10], y = 0.0
//   • Flat lower section : x ∈ [-10, -2],  y = 0.0
//   • Uphill ramp        : x ∈ [ -2, +6],  y rises 0.0 → 1.5
//   • Flat upper green   : x ∈ [  6, +14], y = 1.5
//   • Boulder obstacle   : three rocks across ramp at x ≈ 2
//   • Flag pole          : on the upper green at x = 12
// ─────────────────────────────────────────────────────────────────────────────

Hole3::Hole3() : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // =========================================================================
    // 1. FAIRWAY SECTIONS
    // =========================================================================

    // 1a. Tee mat — yellow mat + white border, sitting on top of lower fairway (y=0.1)
    {
        Mesh* teeMat = new Mesh(Mesh::makeCuboid(1.5f, 0.08f, 1.5f, 1.0f, 0.8f, 0.0f, 1.0f));
        teeMat->translate(-12.0f, 0.1f + 0.04f, 0.0f);
        teeMat->setup();
        addMesh(teeMat);

        Mesh* teeBorder = new Mesh(Mesh::makeCuboid(1.6f, 0.02f, 1.6f, 1.0f, 1.0f, 1.0f, 1.0f));
        teeBorder->translate(-12.0f, 0.1f + 0.02f, 0.0f);
        teeBorder->setup();
        addMesh(teeBorder);
    }

    // 1b. Lower flat section  centre x = (-10 + -2)/2 = -6,  y = 0
    {
        Mesh* m = new Mesh(Mesh::makeCuboid(12.0f, 0.2f, 10.0f,
                                            0.35f, 0.75f, 0.25f, 1.0f));
        m->translate(-8.0f, 0.0f, 0.0f);
        m->setup();
        addMesh(m);
    }

    // 1c. Uphill ramp
    //     makeRamp rotates by -slopeRad around Z, which tilts correctly when
    //     travelling in the -X direction (dx < 0).  So we pass the HIGH end
    //     first (x=+6) and the LOW end second (x=-2) so dx is negative.
    {
        Mesh* m = new Mesh(Mesh::makeRamp2(
        -2.0f, 0.1f, 0.0f,   // top of lower fairway
         6.0f, 1.6f, 0.0f,   // top of upper fairway
        10.0f));
        m->rotate(-90,0.0f, 1.0f, 0.0f);
        m->setup();
        addMesh(m);
    }

    // 1d. Upper flat green  centre x = (6 + 14)/2 = 10,  y = 1.5
    {
        Mesh* m = new Mesh(Mesh::makeCuboid(12.0f, 0.2f, 10.0f,
                                            0.28f, 0.60f, 0.20f, 1.0f));
        m->translate(12.0f, 1.5f, 0.0f);
        m->setup();
        addMesh(m);
    }

    // =========================================================================
    // 2. BOULDER OBSTACLE  (three rocks across ramp at x ≈ 2)
    //    Ramp y at x=2: (2 - (-2)) / (6 - (-2)) * 1.5 = 0.75
    // =========================================================================

    auto addRock = [&](float x, float y, float z, float sc)
    {
        Mesh* m = new Mesh(Mesh::makeRock(sc));
        m->translate(x, y, z);
        m->setup();
        addMesh(m);
    };

    addRock(2.0f, 0.75f + 1.0f, -2.5f, 0.8f);
    addRock(2.5f, 0.75f + 1.0f,  0.0f, 1.0f);
    addRock(2.0f, 0.75f + 1.0f,  2.5f, 0.7f);

    // =========================================================================
    // 3. FLAG POLE — pole (cylinder) + flag tip (cone) on the upper green
    // =========================================================================

    // Pole
    {
        Mesh* m = new Mesh(Mesh::makeCylinder(0.08f, 3.0f, 8,
                                               0.8f, 0.8f, 0.8f, 1.0f));
        m->translate(12.0f, 1.5f + 1.5f, 0.0f);   // base sits on green (y=1.5), centre at +1.5
        m->setup();
        addMesh(m);
    }

    // Flag (small cone angled off the top)
    {
        Mesh* m = new Mesh(Mesh::makeCone(0.5f, 1.0f, 8,
                                           0.9f, 0.1f, 0.1f, 1.0f));
        m->rotate(90.0f, 0.0f, 0.0f, 1.0f);        // lay cone on its side → flag shape
        m->translate(12.3f, 1.5f + 2.8f, 0.0f);
        m->setup();
        addMesh(m);
    }

    // =========================================================================
    // 4. HOLE CUP — small dark cylinder on the upper green
    // =========================================================================
    {
        Mesh* m = new Mesh(Mesh::makeCylinder(0.3f, 0.4f, 16,
                                               0.1f, 0.1f, 0.1f, 1.0f));
        m->translate(12.0f, 1.7f, 0.0f);
        m->setup();
        addMesh(m);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

Hole3::~Hole3()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole3::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole3::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole3::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole3::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}