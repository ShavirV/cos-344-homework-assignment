// =============================================================================
// Drone.cpp
// Implementation of the camera-drone (Report §8).
//
// Reads as a quadcopter:
//
//      ──── front (red LEDs) ────
//        .                     .
//         arm0 ──── O          O ──── arm1
//          .    motor + prop   motor + prop
//           .         /
//            ┌───────────┐
//            │ oct prism │   ← body
//            │     ●     │   ← gimbal sphere (below), torus ring around
//            └───────────┘
//           .         .
//          .           .
//         arm2 ──── O    O ──── arm3
//        .     motor + prop   motor + prop
//      ──── rear (green LEDs) ────
//
// Geometry generated procedurally (no asset files, no prebuilt meshes). All
// CPU-side vertex buffers are uploaded once in init() and the propellers
// then animate via a per-frame spin matrix applied during draw().
// =============================================================================

#include "Drone.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>

// -----------------------------------------------------------------------------
// Geometry constants. Kept in one block so it's easy to scale the whole drone
// uniformly without hunting through the file.
// -----------------------------------------------------------------------------
namespace
{
    // ── Body (flattened octagonal prism, Report §8.1) ────────────────────────
    const float BODY_RADIUS    = 0.55f;   // distance from centre to octagon vertex
    const float BODY_HALF_H    = 0.10f;   // half-thickness of the prism

    // ── Arms ─────────────────────────────────────────────────────────────────
    const float ARM_LENGTH     = 0.80f;   // from body edge to motor centre
    const float ARM_HALF_W     = 0.045f;  // half-width across the arm
    const float ARM_HALF_H     = 0.035f;  // half-thickness vertically

    // ── Motor housings (short cylinders, Report §8.1) ────────────────────────
    const float MOTOR_RADIUS   = 0.10f;
    const float MOTOR_HEIGHT   = 0.08f;
    const int   MOTOR_SEGS     = 16;

    // ── Propellers (two rectangular quads crossed at 90°, Report §8.1) ──────
    const float PROP_LENGTH    = 0.55f;   // tip-to-tip = 2 * (LENGTH/2)
    const float PROP_HALF_W    = 0.025f;  // blade width
    const float PROP_THICK     = 0.005f;  // blade thickness (~paper-thin)

    // ── Camera gimbal (sphere + torus, Report §8.1) ──────────────────────────
    const float GIMBAL_RADIUS  = 0.10f;
    const float GIMBAL_Y       = -BODY_HALF_H - GIMBAL_RADIUS - 0.02f; // below body
    const int   GIMBAL_LAT     = 10;
    const int   GIMBAL_LON     = 16;
    const float TORUS_MAJOR_R  = GIMBAL_RADIUS + 0.04f;
    const float TORUS_TUBE_R   = 0.018f;
    const int   TORUS_MAJOR_S  = 24;
    const int   TORUS_MINOR_S  = 8;

    // ── LEDs (Report §8.4) ───────────────────────────────────────────────────
    const float LED_HALF       = 0.022f;
    const glm::vec3 LED_RED  (1.00f, 0.05f, 0.05f);
    const glm::vec3 LED_GREEN(0.05f, 1.00f, 0.20f);

    // ── Colours (linear RGB) ─────────────────────────────────────────────────
    // Deliberately NOT in the turf-green or kerb-grey buckets so the shader's
    // procedural-noise material detector (in dev.cpp) leaves them alone.
    const glm::vec3 COLOR_BODY    (0.18f, 0.20f, 0.24f);   // dark blue-grey
    const glm::vec3 COLOR_ARM     (0.16f, 0.18f, 0.22f);
    const glm::vec3 COLOR_MOTOR   (0.10f, 0.10f, 0.10f);
    const glm::vec3 COLOR_PROP    (0.32f, 0.32f, 0.34f);
    const glm::vec3 COLOR_GIMBAL  (0.08f, 0.08f, 0.09f);
    const glm::vec3 COLOR_RING    (0.42f, 0.42f, 0.44f);

    // ── Control feel ─────────────────────────────────────────────────────────
    const float MOVE_SPEED      = 12.0f;   // m/s base, multiplied by Ctrl boost
    const float BOOST_FACTOR    = 3.5f;
    const float YAW_SPEED       = 1.6f;    // rad/s (A/D)
    const float PITCH_SPEED     = 1.2f;    // rad/s (W/S tilt)
    const float ROLL_SPEED      = 1.6f;    // rad/s (Q/E)
    const float MOUSE_SENS      = 0.0025f;
    const float PROP_SPIN_SPEED = 40.0f;   // rad/s — visibly spinning
    const float MAX_PITCH       = 1.40f;   // ~80°
    const float MAX_ROLL        = 1.20f;
    const float MAX_CAM_PITCH   = 1.50f;

    // Third-person offsets (Report §8.3.1: d = 5, h = 2).
    const float TP_DISTANCE     = 5.0f;
    const float TP_HEIGHT       = 2.0f;

    // Projection (Report §8.3).
    const float FOV_NORMAL_DEG  = 60.0f;
    const float FOV_ZOOM_DEG    = 20.0f;
    const float NEAR_PLANE      = 0.1f;
    const float FAR_PLANE       = 1000.0f;
}

// =============================================================================
// Construction / destruction
// =============================================================================

Drone::Drone()
    : mTotalIndexCount(0),
      mVAO(0), mVBO(0), mEBO(0),
      mInitialised(false),
      mProgram(0),
      mLocMVP(-1), mLocModel(-1), mLocNormalMat(-1),
      mLocLightDir(-1), mLocLightColor(-1), mLocAmbient(-1), mLocViewPos(-1),
      mPos(0.0f, 8.0f, -15.0f),
      mYaw(0.0f), mPitch(0.0f), mRoll(0.0f),
      mCamYawOffset(0.0f), mCamPitchOffset(-0.25f),
      mPropSpin(0.0f),
      mThirdPerson(false),
      mZoom(false),
      mView(1.0f),
      mEyePos(mPos)
{
    for (int i = 0; i < 4; ++i)
    {
        mProps[i].indexOffset = 0;
        mProps[i].indexCount  = 0;
        mProps[i].hubLocal    = glm::vec3(0.0f);
    }
}

Drone::~Drone()
{
    cleanup();
}

// =============================================================================
// Low-level geometry helpers (mirror Hole13's style)
// =============================================================================

void Drone::pushQuad(const glm::vec3& a, const glm::vec3& b,
                     const glm::vec3& c, const glm::vec3& d,
                     const glm::vec3& n, const glm::vec3& colour)
{
    unsigned int base = (unsigned int)mVertices.size();
    mVertices.push_back({a, n, colour});
    mVertices.push_back({b, n, colour});
    mVertices.push_back({c, n, colour});
    mVertices.push_back({d, n, colour});
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 1);
    mIndices.push_back(base + 2);
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 2);
    mIndices.push_back(base + 3);
}

void Drone::pushTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                    const glm::vec3& n, const glm::vec3& colour)
{
    unsigned int base = (unsigned int)mVertices.size();
    mVertices.push_back({a, n, colour});
    mVertices.push_back({b, n, colour});
    mVertices.push_back({c, n, colour});
    mIndices.push_back(base + 0);
    mIndices.push_back(base + 1);
    mIndices.push_back(base + 2);
}

void Drone::buildBox(const glm::vec3& mn, const glm::vec3& mx,
                     const glm::vec3& colour)
{
    glm::vec3 p000(mn.x, mn.y, mn.z);
    glm::vec3 p100(mx.x, mn.y, mn.z);
    glm::vec3 p110(mx.x, mx.y, mn.z);
    glm::vec3 p010(mn.x, mx.y, mn.z);
    glm::vec3 p001(mn.x, mn.y, mx.z);
    glm::vec3 p101(mx.x, mn.y, mx.z);
    glm::vec3 p111(mx.x, mx.y, mx.z);
    glm::vec3 p011(mn.x, mx.y, mx.z);

    pushQuad(p001, p101, p111, p011, { 0, 0,  1}, colour); // +Z front
    pushQuad(p100, p000, p010, p110, { 0, 0, -1}, colour); // -Z back
    pushQuad(p000, p001, p011, p010, {-1, 0,  0}, colour); // -X left
    pushQuad(p101, p100, p110, p111, { 1, 0,  0}, colour); // +X right
    pushQuad(p011, p111, p110, p010, { 0, 1,  0}, colour); // +Y top
    pushQuad(p000, p100, p101, p001, { 0,-1,  0}, colour); // -Y bottom
}

void Drone::buildLED(const glm::vec3& centre, float halfSize,
                     const glm::vec3& colour)
{
    glm::vec3 mn = centre - glm::vec3(halfSize);
    glm::vec3 mx = centre + glm::vec3(halfSize);
    buildBox(mn, mx, colour);
}

// =============================================================================
// Body — flattened octagonal prism
// =============================================================================
//
// An octagon is laid out in the XZ plane (the "ground plane" of the drone's
// local frame) at y = ±halfH. We compute 8 outer vertices on a circle of
// radius `radius`, then stitch:
//   • Top face — fan of 8 triangles from a central top vertex
//   • Bottom face — same, flipped
//   • Side faces — 8 quads connecting top-ring to bottom-ring
//
// The flat shading is achieved by giving each face its own normal.
void Drone::buildBody(float radius, float halfHeight, const glm::vec3& colour)
{
    const int N = 8;
    glm::vec3 topCentre(0.0f,  halfHeight, 0.0f);
    glm::vec3 botCentre(0.0f, -halfHeight, 0.0f);

    std::vector<glm::vec3> top(N), bot(N);
    for (int i = 0; i < N; ++i)
    {
        // Start at +Z so the "front" of the drone (where front LEDs go) sits
        // on the +Z side. Rotate clockwise looking down (i.e. -angle so the
        // octagon vertices match the diagonal arm directions we choose below).
        float a = (float)i * (2.0f * 3.14159265358979323846f / (float)N);
        float cx = std::cos(a);
        float cz = std::sin(a);
        top[i] = glm::vec3(radius * cx,  halfHeight, radius * cz);
        bot[i] = glm::vec3(radius * cx, -halfHeight, radius * cz);
    }

    // Top fan (CCW seen from +Y).
    for (int i = 0; i < N; ++i)
    {
        int j = (i + 1) % N;
        pushTri(topCentre, top[i], top[j], glm::vec3(0,  1, 0), colour);
    }
    // Bottom fan (CCW seen from -Y → wind the other way).
    for (int i = 0; i < N; ++i)
    {
        int j = (i + 1) % N;
        pushTri(botCentre, bot[j], bot[i], glm::vec3(0, -1, 0), colour);
    }
    // Side quads — each face's normal is the outward XZ direction at its
    // midpoint. Flat shading because all 4 verts of a quad share that normal.
    for (int i = 0; i < N; ++i)
    {
        int j = (i + 1) % N;
        glm::vec3 mid = 0.5f * (top[i] + top[j]);
        glm::vec3 n(mid.x, 0.0f, mid.z);
        if (glm::length(n) > 1e-6f) n = glm::normalize(n);
        // CCW from outside: top[i] → bot[i] → bot[j] → top[j]
        pushQuad(top[i], bot[i], bot[j], top[j], n, colour);
    }
}

// =============================================================================
// Cylinder along +Y (used for motor housings)
// =============================================================================
void Drone::buildCylinderY(const glm::vec3& centre, float r, float h, int segs,
                           const glm::vec3& colour)
{
    float halfH = 0.5f * h;
    glm::vec3 topC = centre + glm::vec3(0.0f,  halfH, 0.0f);
    glm::vec3 botC = centre + glm::vec3(0.0f, -halfH, 0.0f);

    for (int i = 0; i < segs; ++i)
    {
        float a0 = (float)i       * (2.0f * 3.14159265358979323846f / (float)segs);
        float a1 = (float)(i + 1) * (2.0f * 3.14159265358979323846f / (float)segs);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        glm::vec3 t0 = topC + glm::vec3(r * c0, 0.0f, r * s0);
        glm::vec3 t1 = topC + glm::vec3(r * c1, 0.0f, r * s1);
        glm::vec3 b0 = botC + glm::vec3(r * c0, 0.0f, r * s0);
        glm::vec3 b1 = botC + glm::vec3(r * c1, 0.0f, r * s1);

        // Side quad — outward normal is the midpoint direction.
        glm::vec3 nMid(0.5f * (c0 + c1), 0.0f, 0.5f * (s0 + s1));
        if (glm::length(nMid) > 1e-6f) nMid = glm::normalize(nMid);
        pushQuad(t0, b0, b1, t1, nMid, colour);

        // Top cap triangle.
        pushTri(topC, t0, t1, glm::vec3(0,  1, 0), colour);
        // Bottom cap triangle (reverse winding).
        pushTri(botC, b1, b0, glm::vec3(0, -1, 0), colour);
    }
}

// =============================================================================
// UV-sphere (gimbal)
// =============================================================================
void Drone::buildSphere(const glm::vec3& centre, float r,
                        int latSegs, int lonSegs,
                        const glm::vec3& colour)
{
    const float PI = 3.14159265358979323846f;
    // For each lat/lon quad, compute its 4 corners and a per-quad averaged
    // normal so the shading reads as smooth-ish without per-vertex normals.
    for (int i = 0; i < latSegs; ++i)
    {
        float v0 = (float)i       / (float)latSegs;
        float v1 = (float)(i + 1) / (float)latSegs;
        float theta0 = PI * v0;         // 0 = +Y pole, PI = -Y pole
        float theta1 = PI * v1;

        for (int j = 0; j < lonSegs; ++j)
        {
            float u0 = (float)j       / (float)lonSegs;
            float u1 = (float)(j + 1) / (float)lonSegs;
            float phi0 = 2.0f * PI * u0;
            float phi1 = 2.0f * PI * u1;

            auto P = [&](float theta, float phi) {
                glm::vec3 dir(std::sin(theta) * std::cos(phi),
                              std::cos(theta),
                              std::sin(theta) * std::sin(phi));
                return centre + r * dir;
            };

            glm::vec3 p00 = P(theta0, phi0);
            glm::vec3 p01 = P(theta0, phi1);
            glm::vec3 p10 = P(theta1, phi0);
            glm::vec3 p11 = P(theta1, phi1);

            // Flat normal = average direction from centre.
            glm::vec3 n = glm::normalize(((p00 + p01 + p10 + p11) * 0.25f) - centre);

            // Skip degenerate triangles at the poles by using triangles there.
            if (i == 0)
            {
                // Triangle fan at +Y pole.
                pushTri(p00, p11, p01, n, colour);  // p00 == p01 at this pole
            }
            else if (i == latSegs - 1)
            {
                pushTri(p00, p10, p11, n, colour);  // p10 == p11 at -Y pole
            }
            else
            {
                pushQuad(p00, p10, p11, p01, n, colour);
            }
        }
    }
}

// =============================================================================
// Torus around +Y axis (gimbal ring)
// =============================================================================
//
// Parameterised as:
//   p(u, v) = centre + ((R + r·cos v) · cos u, r·sin v, (R + r·cos v) · sin u)
//   u sweeps the major ring around +Y, v sweeps the minor cross-section.
void Drone::buildTorusY(const glm::vec3& centre, float R, float r,
                        int majorSegs, int minorSegs,
                        const glm::vec3& colour)
{
    const float PI = 3.14159265358979323846f;

    auto P = [&](int i, int j) {
        float u = 2.0f * PI * (float)i / (float)majorSegs;
        float v = 2.0f * PI * (float)j / (float)minorSegs;
        float cu = std::cos(u), su = std::sin(u);
        float cv = std::cos(v), sv = std::sin(v);
        glm::vec3 p((R + r * cv) * cu,
                    r * sv,
                    (R + r * cv) * su);
        return centre + p;
    };
    auto N = [&](int i, int j) {
        float u = 2.0f * PI * (float)i / (float)majorSegs;
        float v = 2.0f * PI * (float)j / (float)minorSegs;
        float cu = std::cos(u), su = std::sin(u);
        float cv = std::cos(v), sv = std::sin(v);
        return glm::vec3(cv * cu, sv, cv * su);
    };

    for (int i = 0; i < majorSegs; ++i)
    {
        int i2 = (i + 1) % majorSegs;
        for (int j = 0; j < minorSegs; ++j)
        {
            int j2 = (j + 1) % minorSegs;
            glm::vec3 p00 = P(i,  j );
            glm::vec3 p10 = P(i2, j );
            glm::vec3 p11 = P(i2, j2);
            glm::vec3 p01 = P(i,  j2);
            // Flat normal = average of corner normals — good enough at this
            // tessellation level.
            glm::vec3 n = glm::normalize(N(i, j) + N(i2, j) + N(i2, j2) + N(i, j2));
            pushQuad(p00, p10, p11, p01, n, colour);
        }
    }
}

// =============================================================================
// One arm + motor housing + propeller + LED at the tip
// =============================================================================
//
// `tipDir` must be a unit vector in the XZ plane pointing from the body
// centre toward where the motor sits. We build:
//   • A rectangular-prism arm running from the body edge to the motor.
//     The arm is oriented along tipDir; its "long" axis is tipDir, with
//     a perpendicular sideDir (in XZ) defining its width and Y defining
//     its thickness.
//   • A cylinder motor housing centred at the arm tip.
//   • Two crossed rectangular blades on top of the motor (registered with
//     mProps[propIndex] so update() can spin them).
//   • A single small LED-coloured cube next to the motor.
void Drone::buildArm(const glm::vec3& tipDir,
                     float armLen, float armHalfW, float armHalfH,
                     float motorR, float motorH,
                     float propLen, float propHalfW,
                     const glm::vec3& armColour,
                     const glm::vec3& motorColour,
                     const glm::vec3& propColour,
                     const glm::vec3& ledColour,
                     int   propIndex)
{
    // Side direction in XZ — rotate tipDir 90° around +Y.
    glm::vec3 side(-tipDir.z, 0.0f, tipDir.x);

    // Arm extends from `start` (at the body's edge) to `end` (at the motor
    // centre). The body's edge along tipDir sits at radius BODY_RADIUS.
    glm::vec3 start = BODY_RADIUS * tipDir;
    start.y = 0.0f;
    glm::vec3 end   = start + armLen * tipDir;
    end.y = 0.0f;

    // Build the arm as 8 vertices of a rectangular prism, then 6 quads.
    // Vertices: top/bottom × inner/outer × left/right
    glm::vec3 up(0.0f, armHalfH, 0.0f);
    glm::vec3 dn = -up;
    glm::vec3 lf = -armHalfW * side;
    glm::vec3 rt =  armHalfW * side;

    glm::vec3 sTL = start + up + lf;
    glm::vec3 sTR = start + up + rt;
    glm::vec3 sBL = start + dn + lf;
    glm::vec3 sBR = start + dn + rt;
    glm::vec3 eTL = end   + up + lf;
    glm::vec3 eTR = end   + up + rt;
    glm::vec3 eBL = end   + dn + lf;
    glm::vec3 eBR = end   + dn + rt;

    // Six faces — each with its own normal so the arm flat-shades.
    pushQuad(sTL, sTR, eTR, eTL, glm::vec3(0,  1, 0), armColour);              // top
    pushQuad(sBR, sBL, eBL, eBR, glm::vec3(0, -1, 0), armColour);              // bottom
    pushQuad(sTR, sBR, eBR, eTR, glm::normalize(side), armColour);             // +side
    pushQuad(sBL, sTL, eTL, eBL, glm::normalize(-side), armColour);            // -side
    pushQuad(sTL, sBL, sBR, sTR, glm::normalize(-tipDir), armColour);          // inner cap
    pushQuad(eTR, eBR, eBL, eTL, glm::normalize(tipDir), armColour);           // outer cap

    // Motor housing at the arm tip.
    glm::vec3 motorCentre = end + glm::vec3(0.0f, armHalfH + motorH * 0.5f, 0.0f);
    buildCylinderY(motorCentre, motorR, motorH, MOTOR_SEGS, motorColour);

    // ── LED ──────────────────────────────────────────────────────────────────
    // Placed on the OUTER face of the arm, just below the motor — visible
    // from below + outside in 3rd-person.
    glm::vec3 ledCentre = end + (-armHalfH - LED_HALF - 0.005f) * glm::vec3(0, 1, 0);
    buildLED(ledCentre, LED_HALF, ledColour);

    // Record where the propeller starts BEFORE pushing it.
    // The propeller is built LAST in this arm's sequence so the index range
    // is contiguous — that lets draw() handle it cleanly by drawing the
    // static range first (body + all arms + motors + LEDs in one call) and
    // then each of the 4 prop ranges separately with their spin transform.
    GLuint propStart = (GLuint)mIndices.size();

    // ── Propeller ────────────────────────────────────────────────────────────
    // Built directly above the motor (top face at y = motorTopY + small gap).
    // Two crossed blades: blade A long-axis along local X, blade B along
    // local Z. Both share the same hub (the motor centre in XZ) so the spin
    // applied at draw time rotates them together.
    float motorTopY = motorCentre.y + motorH * 0.5f;
    float propY     = motorTopY + 0.01f;
    glm::vec3 hub(end.x, propY + PROP_THICK * 0.5f, end.z);

    // Blade A — long axis = local X.
    {
        glm::vec3 mn(hub.x - PROP_LENGTH * 0.5f,
                     propY,
                     hub.z - PROP_HALF_W);
        glm::vec3 mx(hub.x + PROP_LENGTH * 0.5f,
                     propY + PROP_THICK,
                     hub.z + PROP_HALF_W);
        buildBox(mn, mx, propColour);
    }
    // Blade B — long axis = local Z, perpendicular to blade A.
    {
        glm::vec3 mn(hub.x - PROP_HALF_W,
                     propY,
                     hub.z - PROP_LENGTH * 0.5f);
        glm::vec3 mx(hub.x + PROP_HALF_W,
                     propY + PROP_THICK,
                     hub.z + PROP_LENGTH * 0.5f);
        buildBox(mn, mx, propColour);
    }

    GLuint propEnd = (GLuint)mIndices.size();

    mProps[propIndex].indexOffset = propStart;
    mProps[propIndex].indexCount  = propEnd - propStart;
    mProps[propIndex].hubLocal    = hub;
    // (void)propLen — propLen is here for future per-arm variation.
    (void)propLen;
}

// =============================================================================
// init — build all CPU-side geometry, upload to GL, cache uniforms
// =============================================================================

void Drone::init(GLuint shaderProgram)
{
    if (mInitialised) return;

    mProgram = shaderProgram;

    // Cache uniform locations. uMVP / uLightColor / uAmbient are not
    // cached here anymore — dev.cpp sets view/projection/lightColor/ambient
    // once per frame as program-wide uniforms, before any of the hole or
    // drone draw() calls.
    mLocModel      = glGetUniformLocation(mProgram, "uModel");
    mLocNormalMat  = glGetUniformLocation(mProgram, "uNormalMat");
    mLocLightDir   = glGetUniformLocation(mProgram, "uLightDir");
    mLocViewPos    = glGetUniformLocation(mProgram, "uViewPos");

    mVertices.clear();
    mIndices.clear();

    // ── 1. Body (octagonal prism) ────────────────────────────────────────────
    buildBody(BODY_RADIUS, BODY_HALF_H, COLOR_BODY);

    // ── 2. Gimbal — sphere + torus, below the body ──────────────────────────
    buildSphere(glm::vec3(0.0f, GIMBAL_Y, 0.0f),
                GIMBAL_RADIUS, GIMBAL_LAT, GIMBAL_LON, COLOR_GIMBAL);
    buildTorusY(glm::vec3(0.0f, GIMBAL_Y, 0.0f),
                TORUS_MAJOR_R, TORUS_TUBE_R,
                TORUS_MAJOR_S, TORUS_MINOR_S, COLOR_RING);

    // ── 3. Four arms (diagonals at ±45° from +Z and +X) ──────────────────────
    // Front arms (FRONT-RIGHT, FRONT-LEFT) get RED LEDs (Report §8.4).
    // Rear arms get GREEN.
    const float k = 0.70710678f; // cos(45°) = sin(45°)
    struct ArmSpec { glm::vec3 dir; glm::vec3 led; int index; };
    ArmSpec arms[4] = {
        { glm::vec3( k, 0,  k), LED_RED,   0 },   // front-right
        { glm::vec3(-k, 0,  k), LED_RED,   1 },   // front-left
        { glm::vec3(-k, 0, -k), LED_GREEN, 2 },   // rear-left
        { glm::vec3( k, 0, -k), LED_GREEN, 3 },   // rear-right
    };
    for (int i = 0; i < 4; ++i)
    {
        buildArm(arms[i].dir,
                 ARM_LENGTH, ARM_HALF_W, ARM_HALF_H,
                 MOTOR_RADIUS, MOTOR_HEIGHT,
                 PROP_LENGTH, PROP_HALF_W,
                 COLOR_ARM, COLOR_MOTOR, COLOR_PROP, arms[i].led,
                 arms[i].index);
    }

    // ── 4. Upload to GPU ─────────────────────────────────────────────────────
    if (mVertices.empty() || mIndices.empty())
    {
        std::cerr << "Drone::init — no geometry generated!" << std::endl;
        return;
    }

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 mVertices.size() * sizeof(Vertex),
                 mVertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mIndices.size() * sizeof(unsigned int),
                 mIndices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mTotalIndexCount = (GLuint)mIndices.size();

    // The CPU copy is no longer needed after the GPU upload.
    mVertices.clear(); mVertices.shrink_to_fit();
    mIndices.clear();  mIndices.shrink_to_fit();

    mInitialised = true;
}

// =============================================================================
// Orientation helpers
// =============================================================================

glm::mat4 Drone::bodyModel() const
{
    glm::mat4 M(1.0f);
    M = glm::translate(M, mPos);
    M = glm::rotate(M, mYaw,   glm::vec3(0, 1, 0));   // world Y
    M = glm::rotate(M, mPitch, glm::vec3(1, 0, 0));   // local X
    M = glm::rotate(M, mRoll,  glm::vec3(0, 0, 1));   // local Z
    return M;
}

glm::vec3 Drone::bodyForward() const
{
    // Apply body rotations to (0, 0, 1).
    glm::mat4 R(1.0f);
    R = glm::rotate(R, mYaw,   glm::vec3(0, 1, 0));
    R = glm::rotate(R, mPitch, glm::vec3(1, 0, 0));
    R = glm::rotate(R, mRoll,  glm::vec3(0, 0, 1));
    glm::vec4 f = R * glm::vec4(0, 0, 1, 0);
    return glm::normalize(glm::vec3(f));
}

glm::vec3 Drone::bodyRight() const
{
    glm::mat4 R(1.0f);
    R = glm::rotate(R, mYaw,   glm::vec3(0, 1, 0));
    R = glm::rotate(R, mPitch, glm::vec3(1, 0, 0));
    R = glm::rotate(R, mRoll,  glm::vec3(0, 0, 1));
    glm::vec4 r = R * glm::vec4(1, 0, 0, 0);
    return glm::normalize(glm::vec3(r));
}

glm::vec3 Drone::bodyUp() const
{
    glm::mat4 R(1.0f);
    R = glm::rotate(R, mYaw,   glm::vec3(0, 1, 0));
    R = glm::rotate(R, mPitch, glm::vec3(1, 0, 0));
    R = glm::rotate(R, mRoll,  glm::vec3(0, 0, 1));
    glm::vec4 u = R * glm::vec4(0, 1, 0, 0);
    return glm::normalize(glm::vec3(u));
}

// =============================================================================
// Projection
// =============================================================================

glm::mat4 Drone::projection(float aspect) const
{
    float fovDeg = mZoom ? FOV_ZOOM_DEG : FOV_NORMAL_DEG;
    return glm::perspective(glm::radians(fovDeg), aspect, NEAR_PLANE, FAR_PLANE);
}

// =============================================================================
// Per-frame update — controls + matrices (Report §8.2 / §8.3 / §8.3.1)
// =============================================================================

void Drone::update(GLFWwindow* window, float dt, float mouseDx, float mouseDy)
{
    if (!window) return;

    // ── 1. Mouse-look (1st-person only — locked in 3rd-person per §8.3.1) ──
    if (!mThirdPerson)
    {
        mCamYawOffset   += mouseDx * MOUSE_SENS;
        mCamPitchOffset -= mouseDy * MOUSE_SENS;
        if (mCamPitchOffset >  MAX_CAM_PITCH) mCamPitchOffset =  MAX_CAM_PITCH;
        if (mCamPitchOffset < -MAX_CAM_PITCH) mCamPitchOffset = -MAX_CAM_PITCH;
    }

    // ── 2. Body controls (Report §8.2) ───────────────────────────────────────
    bool boost = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    float moveSpeed = MOVE_SPEED * (boost ? BOOST_FACTOR : 1.0f);

    // A / D — yaw left / right (around world +Y).
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) mYaw -= YAW_SPEED * dt;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) mYaw += YAW_SPEED * dt;

    // Q / E — roll left / right (around local +Z).
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) mRoll -= ROLL_SPEED * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) mRoll += ROLL_SPEED * dt;

    // W / S — tilt forward / back AND translate along local forward.
    // The report describes W/S as "Pitch forward / backward (tilt and
    // translate in the drone's local forward direction)" — so we tilt the
    // body while we're holding the key, and translate along the (now
    // forward-flat) direction. The translation uses an XZ-plane projection
    // of forward so the drone doesn't drift up/down when pitched.
    bool fwd = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool bwd = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    if (fwd) mPitch -= PITCH_SPEED * 0.4f * dt;     // nose down
    if (bwd) mPitch += PITCH_SPEED * 0.4f * dt;     // nose up
    // Auto-level pitch & roll when no input — gives the "settles to hover"
    // feel real quadcopters have, and prevents the drone tumbling.
    if (!fwd && !bwd) mPitch *= (1.0f - std::min(1.0f, 2.0f * dt));
    if (glfwGetKey(window, GLFW_KEY_Q) != GLFW_PRESS &&
        glfwGetKey(window, GLFW_KEY_E) != GLFW_PRESS)
    {
        mRoll *= (1.0f - std::min(1.0f, 2.0f * dt));
    }

    // Clamp pitch / roll so the maths stays well-behaved.
    if (mPitch >  MAX_PITCH) mPitch =  MAX_PITCH;
    if (mPitch < -MAX_PITCH) mPitch = -MAX_PITCH;
    if (mRoll  >  MAX_ROLL)  mRoll  =  MAX_ROLL;
    if (mRoll  < -MAX_ROLL)  mRoll  = -MAX_ROLL;

    // Translation. Use the yaw-only forward / right so steering stays
    // horizontal even when pitched (matches typical mini-golf "fly camera"
    // feel — the drone moves where it's *facing*, not where it's *tilted*).
    glm::vec3 fwdFlat(std::sin(mYaw), 0.0f, std::cos(mYaw));
    glm::vec3 rgtFlat(std::cos(mYaw), 0.0f, -std::sin(mYaw));
    if (fwd) mPos += fwdFlat * moveSpeed * dt;
    if (bwd) mPos -= fwdFlat * moveSpeed * dt;

    // Space / Shift — vertical movement (world up).
    if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) mPos.y += moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) mPos.y -= moveSpeed * dt;

    // Z — zoom (Report §8.3 optional zoom).
    mZoom = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;

    // (void)rgtFlat — held in scope in case we add A/D strafe later;
    // current bindings reserve A/D for yaw per the report.
    (void)rgtFlat;

    // ── 3. Propeller spin (visual only) ──────────────────────────────────────
    mPropSpin += PROP_SPIN_SPEED * dt;
    // Keep the value bounded so float precision stays good over long runs.
    const float TWO_PI = 6.28318530717958647692f;
    if (mPropSpin > 1e4f) mPropSpin = std::fmod(mPropSpin, TWO_PI);

    // ── 4. Build view + eye position ─────────────────────────────────────────
    if (mThirdPerson)
    {
        // Third-person camera (Report §8.3.1):
        //   forward = (sin θ, 0, cos θ)
        //   cameraPos = P − d·forward + (0, h, 0)
        glm::vec3 fwdYaw(std::sin(mYaw), 0.0f, std::cos(mYaw));
        glm::vec3 camPos = mPos - TP_DISTANCE * fwdYaw + glm::vec3(0.0f, TP_HEIGHT, 0.0f);
        mView   = glm::lookAt(camPos, mPos, glm::vec3(0, 1, 0));
        mEyePos = camPos;
    }
    else
    {
        // First-person camera. The eye sits at the gimbal position, and the
        // look direction is the drone's body forward rotated by the mouse-
        // look offsets (independent per §8.2).
        glm::mat4 bodyM = bodyModel();
        glm::vec4 eyeLocal(0.0f, GIMBAL_Y, 0.0f, 1.0f);
        glm::vec3 eye = glm::vec3(bodyM * eyeLocal);

        // Build look direction. Start from body forward, then apply the mouse
        // pitch/yaw offsets around the body's up / right axes.
        glm::vec3 fwd = bodyForward();
        glm::vec3 up  = bodyUp();
        glm::vec3 rgt = bodyRight();

        // Yaw offset around body up.
        glm::mat4 RY = glm::rotate(glm::mat4(1.0f), mCamYawOffset, up);
        // Pitch offset around body right.
        glm::mat4 RP = glm::rotate(glm::mat4(1.0f), mCamPitchOffset, rgt);
        glm::vec3 lookDir = glm::vec3(RP * RY * glm::vec4(fwd, 0.0f));
        lookDir = glm::normalize(lookDir);

        mView   = glm::lookAt(eye, eye + lookDir, glm::vec3(0, 1, 0));
        mEyePos = eye;
    }
}

// =============================================================================
// frameOn — position the drone behind & above a target, looking at it
// =============================================================================

void Drone::frameOn(const glm::vec3& target, float radius)
{
    // Approach from "south" (-Z) at an elevation proportional to the scene
    // radius — same logic as the existing dev.cpp Camera::frameOn.
    mPos   = glm::vec3(target.x,
                       target.y + radius * 0.55f,
                       target.z - radius * 0.95f);
    mYaw   = 0.0f;       // facing +Z (toward the scene)
    mPitch = 0.0f;
    mRoll  = 0.0f;

    glm::vec3 dir = glm::normalize(target - mPos);
    // Set the camera mouse-look offsets so the 1st-person view looks at the
    // target. (The drone body itself stays level for a clean hover pose.)
    mCamPitchOffset = std::asin(dir.y);
    mCamYawOffset   = std::atan2(dir.x, dir.z);
}

// =============================================================================
// onKey — handle one-off key presses (V toggle)
// =============================================================================

void Drone::onKey(int key, int action)
{
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_V) mThirdPerson = !mThirdPerson;
}

// =============================================================================
// draw — render the drone body (3rd-person only)
// =============================================================================
//
// EBO layout after init():
//   [0, mBodyIndexCount)         — body + gimbal + ring  (all static)
//   [mBodyIndexCount, P0.start)  — arm0 + motor0 + LED0  (static)
//   [P0.start, P0.end)           — propeller 0           (spinning)
//   [P0.end,    P1.start)        — arm1 + motor1 + LED1  (static)
//   ... and so on through arm3 + prop3 ...
//   [P3.end, mTotalIndexCount)   — (empty — nothing after the last prop)
//
// draw() walks this layout: it submits each static segment under the body
// model matrix, then each propeller under (body · translateToHub · spin ·
// translateFromHub).
void Drone::draw(const glm::mat4& view,
                 const glm::mat4& proj,
                 const glm::vec3& sunDir,
                 const glm::vec3& camPos)
{
    if (!mInitialised)   return;
    if (!mThirdPerson)   return;   // in 1st-person the drone is the camera

    glUseProgram(mProgram);

    if (mLocLightDir   != -1) glUniform3fv(mLocLightDir, 1, glm::value_ptr(sunDir));
    if (mLocViewPos    != -1) glUniform3fv(mLocViewPos,  1, glm::value_ptr(camPos));

    // view + proj are program-wide; uLightColor + uAmbient are owned by
    // dev.cpp now and set on the program each frame. Suppress unused-param
    // warnings.
    (void)view; (void)proj;

    glBindVertexArray(mVAO);

    auto pushModel = [&](const glm::mat4& M) {
        // No MVP precompute — the vertex shader builds gl_Position from
        // projection * view * uModel itself. We only need uModel and the
        // matching normal matrix here.
        glm::mat3 nrmM = glm::mat3(glm::transpose(glm::inverse(M)));
        if (mLocModel     != -1) glUniformMatrix4fv(mLocModel,     1, GL_FALSE, glm::value_ptr(M));
        if (mLocNormalMat != -1) glUniformMatrix3fv(mLocNormalMat, 1, GL_FALSE, glm::value_ptr(nrmM));
    };

    glm::mat4 bodyM = bodyModel();

    // 1. STATIC SEGMENTS — under the body model matrix.
    //    Walk through gaps between propeller ranges.
    pushModel(bodyM);

    GLuint cursor = 0;
    for (int i = 0; i < 4; ++i)
    {
        const PropMeta& pm = mProps[i];
        // Static gap before this propeller.
        if (pm.indexOffset > cursor)
        {
            GLsizei chunk = (GLsizei)(pm.indexOffset - cursor);
            glDrawElements(GL_TRIANGLES,
                           chunk,
                           GL_UNSIGNED_INT,
                           (void*)(uintptr_t)(cursor * sizeof(unsigned int)));
        }
        cursor = pm.indexOffset + pm.indexCount;
    }
    // Tail after the last propeller (would be empty under current init()
    // order, but kept for robustness if more geometry is appended later).
    if (mTotalIndexCount > cursor)
    {
        GLsizei chunk = (GLsizei)(mTotalIndexCount - cursor);
        glDrawElements(GL_TRIANGLES,
                       chunk,
                       GL_UNSIGNED_INT,
                       (void*)(uintptr_t)(cursor * sizeof(unsigned int)));
    }

    // 2. PROPELLERS — each under (body · translate(hub) · rotateY(spin) ·
    //    translate(-hub)) so the blades spin in place at their hub.
    //    On a real quadcopter, diagonally-opposite propellers spin in the
    //    same direction and adjacent ones counter-rotate (cancels torque).
    //    We mimic that here: props 0 and 2 turn +spin, props 1 and 3 turn
    //    -spin.
    for (int i = 0; i < 4; ++i)
    {
        const PropMeta& pm = mProps[i];
        if (pm.indexCount == 0) continue;

        float dir = ((i & 1) == 0) ? 1.0f : -1.0f;
        glm::mat4 toHub   = glm::translate(glm::mat4(1.0f), pm.hubLocal);
        glm::mat4 spin    = glm::rotate(glm::mat4(1.0f), mPropSpin * dir, glm::vec3(0,1,0));
        glm::mat4 fromHub = glm::translate(glm::mat4(1.0f), -pm.hubLocal);
        glm::mat4 propModel = bodyM * toHub * spin * fromHub;

        pushModel(propModel);
        glDrawElements(GL_TRIANGLES,
                       (GLsizei)pm.indexCount,
                       GL_UNSIGNED_INT,
                       (void*)(uintptr_t)(pm.indexOffset * sizeof(unsigned int)));
    }

    glBindVertexArray(0);
}

// =============================================================================
// cleanup
// =============================================================================

void Drone::cleanup()
{
    if (mEBO) { glDeleteBuffers(1, &mEBO); mEBO = 0; }
    if (mVBO) { glDeleteBuffers(1, &mVBO); mVBO = 0; }
    if (mVAO) { glDeleteVertexArrays(1, &mVAO); mVAO = 0; }
    mInitialised = false;
}