#include "Hole6.h"

// ─────────────────────────────────────────────────────────────────────────────
// Hole 6 — Swamp & Waterfall Zone (bridge crossing)
//
// The stream runs along Z ≈ 0 (across the width of the fairway, i.e. parallel
// to the Z axis).  The wooden bridge spans it at roughly X = -2 so the player
// walks from the tee (-X end) over the bridge to reach the cup (+X end).
//
// Bridge structure (all Y offsets relative to fairway top surface = 0.1):
//   deck     : wide flat cuboid above the stream
//   planks   : 5 thin strips on top of the deck, slightly raised, brown
//   post L/R : 4 thin cylinders (2 per long side) rising from deck corners
//   handrail : 2 horizontal thin cylinders connecting each pair of posts
//
// Waterfall continuation: 2-tier concrete ledge mirroring Hole 5's wall
//   on the +Z side.
// ─────────────────────────────────────────────────────────────────────────────

Hole6::Hole6()
    : meshCount(0)
{
    for (int i = 0; i < MAX_MESHES; i++)
        meshes[i] = nullptr;

    // ── Fairway + 4 concrete borders ─────────────────────────────────────────
    Mesh::buildHoleMeshes(0.0f, meshes, meshCount, MAX_MESHES);

    // ── Stream channel (crosses fairway along Z, centred near X = -2) ────────
    Mesh* stream = new Mesh(Mesh::makeCuboid(4.5f, 0.05f, 15.0f,
                                              0.10f, 0.55f, 0.45f, 0.75f));
    stream->translate(-2.0f, 0.13f, 0.0f);
    stream->setup();
    addMesh(stream);

    // ── Bridge ────────────────────────────────────────────────────────────────
    // Deck: spans the stream, sits just above fairway surface
    const float DECK_Y = 0.35f;   // height above world origin (on top of fairway+stream)
    Mesh* deck = new Mesh(Mesh::makeCuboid(5.0f, 0.2f, 6.0f,
                                            0.55f, 0.35f, 0.15f, 1.0f));
    deck->translate(-2.0f, DECK_Y, 0.0f);
    deck->setup();
    addMesh(deck);

    // Planks: 5 thin strips across the deck (run along X, spaced along Z)
    const float PLANK_Y = DECK_Y + 0.12f;
    float plankZPositions[5] = { -2.2f, -1.1f, 0.0f, 1.1f, 2.2f };
    for (int p = 0; p < 5; p++)
    {
        Mesh* plank = new Mesh(Mesh::makeCuboid(5.0f, 0.1f, 0.6f,
                                                 0.45f, 0.28f, 0.10f, 1.0f));
        plank->translate(-2.0f, PLANK_Y, plankZPositions[p]);
        plank->setup();
        addMesh(plank);
    }

    // Railing posts: 4 thin cylinders at the deck corners (2 on each Z side)
    const float POST_R  = 0.12f;
    const float POST_H  = 1.2f;
    const float POST_Y  = DECK_Y + POST_H * 0.5f + 0.1f;
    float postX[2] = { -4.3f, 0.3f };   // near and far X along the bridge
    float postZ[2] = { -2.6f, 2.6f };   // ±Z edges of the deck

    for (int px = 0; px < 2; px++)
    {
        for (int pz = 0; pz < 2; pz++)
        {
            Mesh* post = new Mesh(Mesh::makeCylinder(POST_R, POST_H, 8,
                                                      0.45f, 0.28f, 0.10f, 1.0f));
            post->translate(postX[px], POST_Y, postZ[pz]);
            post->setup();
            addMesh(post);
        }
    }

    // Handrails: 2 horizontal thin cylinders connecting posts on each Z side
    const float RAIL_Y = DECK_Y + POST_H + 0.1f;
    for (int side = 0; side < 2; side++)
    {
        // Cylinder running along X between the two posts on the same Z side.
        // makeCylinder stands up along Y, so we rotate 90° around Z to lay it
        // along X.
        Mesh* rail = new Mesh(Mesh::makeCylinder(0.08f, 4.6f, 8,
                                                   0.45f, 0.28f, 0.10f, 1.0f));
        rail->rotate(90.0f, 0.0f, 0.0f, 1.0f);   // lay along X axis
        rail->translate(-2.0f, RAIL_Y, postZ[side]);
        rail->setup();
        addMesh(rail);
    }

    // ── Waterfall continuation — 2 tiers on +Z side ──────────────────────────
    Mesh* wf1 = new Mesh(Mesh::makeCuboid(18.0f, 1.0f, 1.2f,
                                           0.60f, 0.60f, 0.60f, 1.0f));
    wf1->translate(0.0f, 0.5f, 9.6f);
    wf1->setup();
    addMesh(wf1);

    Mesh* wf2 = new Mesh(Mesh::makeCuboid(10.0f, 1.0f, 0.9f,
                                           0.65f, 0.65f, 0.65f, 1.0f));
    wf2->translate(0.0f, 1.5f, 9.65f);
    wf2->setup();
    addMesh(wf2);

    // ── Rock obstacle — guards the bridge approach ────────────────────────────
    Mesh* rock = new Mesh(Mesh::makeRock(1.0f));
    rock->translate(-5.5f, 0.2f, 4.0f);
    rock->setup();
    addMesh(rock);

    // ── Palm tree at the cup end (+X side) ───────────────────────────────────
    Mesh* trunk = new Mesh(Mesh::makeCylinder(0.4f, 4.5f, 10,
                                               0.36f, 0.25f, 0.13f, 1.0f));
    trunk->translate(11.5f, 2.25f, -5.5f);
    trunk->setup();
    addMesh(trunk);

    Mesh* canopy = new Mesh(Mesh::makeCone(2.5f, 5.0f, 14,
                                            0.10f, 0.45f, 0.18f, 1.0f));
    canopy->translate(11.5f, 6.5f, -5.5f);
    canopy->setup();
    addMesh(canopy);
}

Hole6::~Hole6()
{
    for (int i = 0; i < meshCount; i++)
    {
        delete meshes[i];
        meshes[i] = nullptr;
    }
}

void Hole6::addMesh(Mesh* m)
{
    if (meshCount < MAX_MESHES)
        meshes[meshCount++] = m;
}

void Hole6::draw(GLuint shaderProgram, bool wireframe)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->draw(shaderProgram, wireframe);
}

void Hole6::translate(float x, float y, float z)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->translate(x, y, z);
}

void Hole6::rotate(float angle, float ax, float ay, float az)
{
    for (int i = 0; i < meshCount; i++)
        meshes[i]->rotate(angle, ax, ay, az);
}