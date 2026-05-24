#pragma once
// =============================================================================
// Drone.h
// The camera-drone for the COS344 mini-golf course (Report §8).
//
// Geometry (Report §8.1)
// ----------------------
//   • Body: flattened octagonal prism (flat-shaded polygon faces).
//   • Arms: four diagonal arms (thin rectangular prisms) extending from the
//     body's corners.
//   • Motor housings: short cylinders at each arm tip.
//   • Propellers: two thin rectangular quad "blades" per motor, crossed at
//     90°, spinning continuously to portray flight.
//   • Camera gimbal: a small sphere beneath the body, encircled by a torus
//     ring (the stabilisation frame).
//   • LEDs (Report §8.4): two red on the front arms, two green on the rear.
//
// The drone OWNS the camera. It exposes view()/projection() and a draw()
// for third-person rendering. In first-person mode the drone body is not
// drawn (the camera sits at the gimbal looking forward).
//
// Controls (Report §8.2) — handled in update():
//   W / S        — pitch forward / backward (tilts + translates along local fwd)
//   A / D        — yaw left / right (world-up rotation)
//   Q / E        — roll left / right (around local forward axis)
//   Space / LShift — ascend / descend (world-up translation)
//   Mouse        — camera pitch/yaw (independent in 1st-person, locked in 3rd)
//   V            — toggle 1st-person / 3rd-person view
//   Z            — hold to zoom (FoV 60° → 20°, Report §8.3)
//
// Projection (Report §8.3): perspective, FoV 60°, near 0.1, far 1000.
//
// Third-person derivation (Report §8.3.1):
//   forward = (sin θ, 0, cos θ)
//   cameraPos = P − d·forward + (0, h, 0)     with d = 5, h = 2
//   view = lookAt(cameraPos, P, +Y)
//
// Only spec-allowed libraries are used (GL/glew.h, GLFW/glfw3.h, glm,
// standard C++).
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

class Drone
{
public:
    Drone();
    ~Drone();

    // Build geometry, cache uniforms. Call once after GL context exists.
    void init(GLuint shaderProgram);

    // Per-frame update — reads inputs from the GLFW window, advances
    // position / orientation / propeller spin.
    //
    //   window   — the GLFW window to read keyboard state from
    //   dt       — seconds since last call
    //   mouseDx  — horizontal mouse delta in pixels since last call
    //   mouseDy  — vertical mouse delta in pixels since last call
    void update(GLFWwindow* window, float dt, float mouseDx, float mouseDy);

    // Draw the drone body. Only does anything in 3rd-person mode (otherwise
    // the body would clip into the view). Safe to call every frame.
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& sunDir,
              const glm::vec3& camPos);

    // Release GL resources.
    void cleanup();

    // Camera matrices (recomputed inside update()).
    glm::mat4 view()       const { return mView; }
    glm::mat4 projection(float aspect) const;

    // Position of the *eye* (where the view originates) — what the fragment
    // shader's uViewPos uniform should be fed for correct specular highlights.
    glm::vec3 eyePos() const { return mEyePos; }

    // Frame the drone on a target (used when the user switches holes).
    // Places the drone slightly above and behind the target, looking at it.
    void frameOn(const glm::vec3& target, float radius);

    // Manual key handling for GLFW key callbacks (toggle 1st/3rd person).
    // Pass GLFW_KEY_V / GLFW_KEY_Z etc; ignored unless relevant.
    void onKey(int key, int action);

    // Snapshot getters (used by the dev viewer's HUD / debug output).
    glm::vec3 position() const { return mPos; }
    float     yaw()      const { return mYaw;   }
    float     pitch()    const { return mPitch; }
    float     roll()     const { return mRoll;  }
    bool      thirdPerson() const { return mThirdPerson; }

private:
    // -------------------------------------------------------------------------
    // Vertex layout — identical to Hole13/14/15 so the shared shader works
    // (aPos = 0, aNormal = 1, aColor = 2).
    // -------------------------------------------------------------------------
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
    };

    // -------------------------------------------------------------------------
    // Geometry builders — all in DRONE-LOCAL space, centred on the origin.
    //
    // The drone's local frame:
    //   +X — right
    //   +Y — up
    //   +Z — forward (the direction the camera looks in 1st-person)
    //
    // The propellers are tracked by index so update() can spin them by
    // applying a per-propeller rotation matrix during draw().
    // -------------------------------------------------------------------------

    // Flattened octagonal prism (the drone body).
    void buildBody(float radius, float halfHeight, const glm::vec3& colour);

    // One arm + motor housing + propeller + LED at the tip.
    //   tipDir is the unit XZ-plane direction from the body centre toward
    //   the arm tip (each of the four diagonals).
    void buildArm(const glm::vec3& tipDir,
                  float armLen, float armHalfW, float armHalfH,
                  float motorR, float motorH,
                  float propLen, float propHalfW,
                  const glm::vec3& armColour,
                  const glm::vec3& motorColour,
                  const glm::vec3& propColour,
                  const glm::vec3& ledColour,
                  int   propIndex);    // 0..3 — recorded for spin

    // Cylinder centred on `centre`, axis along +Y, radius `r`, height `h`.
    // Generates side faces (`segs` quads) + top + bottom caps.
    void buildCylinderY(const glm::vec3& centre, float r, float h, int segs,
                        const glm::vec3& colour);

    // Sphere (UV sphere) at `centre`, radius `r`, `latSegs` × `lonSegs`.
    void buildSphere(const glm::vec3& centre, float r,
                     int latSegs, int lonSegs,
                     const glm::vec3& colour);

    // Torus around +Y axis, centred at `centre`. Major radius R, tube radius r,
    // `majorSegs` along the ring, `minorSegs` around the tube cross-section.
    void buildTorusY(const glm::vec3& centre, float R, float r,
                     int majorSegs, int minorSegs,
                     const glm::vec3& colour);

    // Axis-aligned box helper (used for arms and propeller blades).
    void buildBox(const glm::vec3& mn, const glm::vec3& mx,
                  const glm::vec3& colour);

    // Small "LED" cube — tiny emissive-looking box at the tip of an arm.
    // (Treated as a normal lit fragment by the existing shader; the bright
    // saturated colour reads well enough in both day and night modes for
    // the unshaded LED look the report asks for.)
    void buildLED(const glm::vec3& centre, float halfSize,
                  const glm::vec3& colour);

    // Push a single quad with a per-quad normal. CCW from the +n side.
    void pushQuad(const glm::vec3& a, const glm::vec3& b,
                  const glm::vec3& c, const glm::vec3& d,
                  const glm::vec3& n, const glm::vec3& colour);

    // Push a triangle with a per-tri normal.
    void pushTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                 const glm::vec3& n, const glm::vec3& colour);

    // -------------------------------------------------------------------------
    // Static-body geometry (everything except the propellers) is uploaded once
    // into one VAO/VBO/EBO. The propellers are uploaded separately because
    // they spin — each frame draw() resets to the body's model matrix, then
    // multiplies by a per-propeller spin rotation around the propeller's
    // local Y-axis. Tracking is done via mPropOffsets / mPropCounts —
    // index buffer offsets and counts for each of the 4 propellers in the
    // same EBO as the body.
    // -------------------------------------------------------------------------
    std::vector<Vertex>       mVertices;
    std::vector<unsigned int> mIndices;

    // For each of the 4 propellers, record: index offset (in indices),
    // index count, and the propeller hub position in local space.
    struct PropMeta {
        GLuint    indexOffset;
        GLuint    indexCount;
        glm::vec3 hubLocal;
    };
    PropMeta mProps[4];

    // For the static (non-propeller) part of the body:
    GLuint mTotalIndexCount;   // total indices uploaded (used by draw)

    // GL resources
    GLuint  mVAO;
    GLuint  mVBO;
    GLuint  mEBO;
    bool    mInitialised;

    // Cached shader program + uniform locations
    GLuint mProgram;
    GLint  mLocMVP;
    GLint  mLocModel;
    GLint  mLocNormalMat;
    GLint  mLocLightDir;
    GLint  mLocLightColor;
    GLint  mLocAmbient;
    GLint  mLocViewPos;

    // -------------------------------------------------------------------------
    // Drone state — position + orientation in world space.
    // Orientation is stored as Euler (yaw around Y, pitch around local X,
    // roll around local Z). This matches the report's control description:
    // A/D yaws (world-up), W/S adjusts pitch *and* translates along local
    // forward, Q/E rolls about local forward.
    //
    // Mouse-look adjusts mCamYawOffset / mCamPitchOffset (independent of the
    // drone body in 1st-person, per Report §8.2: "Mouse movement: Adjust the
    // camera pitch and yaw (independent of drone body orientation in
    // first-person mode)"). In 3rd-person the camera is locked to the
    // drone yaw per §8.3.1 so the offsets are bypassed.
    // -------------------------------------------------------------------------
    glm::vec3 mPos;          // world position of the drone body centre
    float     mYaw;          // radians, around world +Y; 0 → +Z
    float     mPitch;        // radians, around local +X
    float     mRoll;         // radians, around local +Z

    float     mCamYawOffset;     // 1st-person mouse yaw, relative to drone yaw
    float     mCamPitchOffset;   // 1st-person mouse pitch, relative to drone pitch

    float     mPropSpin;     // accumulated propeller angle (radians)

    bool      mThirdPerson;  // V key toggle
    bool      mZoom;         // Z key held

    // Computed each update().
    glm::mat4 mView;
    glm::vec3 mEyePos;

    // -------------------------------------------------------------------------
    // Helpers for orientation maths.
    // -------------------------------------------------------------------------

    // Build the drone-body model matrix in world space from mPos / mYaw /
    // mPitch / mRoll. Order matches the report: yaw (world-Y) first, then
    // pitch about local X, then roll about local Z.
    glm::mat4 bodyModel() const;

    // Forward / right / up unit vectors for the drone body in world space,
    // derived from yaw + pitch + roll.
    glm::vec3 bodyForward() const;
    glm::vec3 bodyRight() const;
    glm::vec3 bodyUp() const;
};