#include "Hole14.h"
#include "HoleMeshBuilder.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace holegeom;

Hole14::Hole14()
    : _vao(0), _vbo(0), _ebo(0), _indexCount(0), _program(0),
      _uMVP(-1), _uModel(-1), _uNormalMat(-1),
      _uLightDir(-1), _uLightColor(-1), _uAmbient(-1), _uViewPos(-1),
      _teePos(0.0f), _cupPos(0.0f), _sceneCenter(0.0f), _sceneRadius(50.0f)
{}

Hole14::~Hole14() { cleanup(); }

void Hole14::init(GLuint shaderProgram)
{
    _program     = shaderProgram;
    _uMVP        = glGetUniformLocation(_program, "uMVP");
    _uModel      = glGetUniformLocation(_program, "uModel");
    _uNormalMat  = glGetUniformLocation(_program, "uNormalMat");
    _uLightDir   = glGetUniformLocation(_program, "uLightDir");
    _uLightColor = glGetUniformLocation(_program, "uLightColor");
    _uAmbient    = glGetUniformLocation(_program, "uAmbient");
    _uViewPos    = glGetUniformLocation(_program, "uViewPos");
    build();
}

void Hole14::build()
{
    MeshBuilder mb;

    // ---- Palette ----
    const float turfA[3]   = { 0.16f, 0.66f, 0.22f };
    const float turfB[3]   = { 0.10f, 0.55f, 0.18f };  // darker green for the pedestal top
    const float rockC[3]   = { 0.20f, 0.18f, 0.18f };
    const float lavaBright[3] = { 1.00f, 0.45f, 0.10f };
    const float lavaDark[3]   = { 0.85f, 0.20f, 0.05f };
    const float concrete[3]= { 0.72f, 0.70f, 0.66f };
    const float darkSand[3]= { 0.28f, 0.23f, 0.20f };

    // ---- Layout ----
    //
    //   tee fairway: x in [-8, 8], z in [0, 22], y = 0
    //   ramp:        x in [-2, 2], z in [22, 32], rises 0 -> 6
    //   pedestal:    centred at (0, *, 50), radius 12, top at y = 6
    //   moat:        ring at y = 0.15, r in [12.5, 17]
    //   channels:    8 radial strips extending outward from r=17 to r=24
    //
    const float teeXMin = -8.0f, teeXMax = 8.0f;
    const float teeZ0   = 0.0f,  teeZ1   = 22.0f;
    const float rampW   = 4.0f;
    const float rampZ0  = 22.0f, rampZ1  = 32.0f;
    const float pedTopY = 6.0f;
    const float pedR    = 12.0f;
    const float pedZ    = 50.0f;

    // ---- Underlying volcanic-ground pad ----
    // Wound CCW from above so the surface normal points +Y.
    addFlatQuad(mb,
                -30.0f, -0.05f, -6.0f,
                -30.0f, -0.05f, 78.0f,
                 30.0f, -0.05f, 78.0f,
                 30.0f, -0.05f, -6.0f,
                darkSand[0], darkSand[1], darkSand[2]);

    // ---- Tee fairway (flat turf) ----
    // CCW from above
    addFlatQuad(mb,
                teeXMin, 0.05f, teeZ0,
                teeXMin, 0.05f, teeZ1,
                teeXMax, 0.05f, teeZ1,
                teeXMax, 0.05f, teeZ0,
                turfA[0], turfA[1], turfA[2]);

    // ---- Rock-formation borders of the tee fairway ----
    addRockFormation(mb,
                     teeXMin - 0.5f, teeZ0 - 0.5f,
                     teeXMin - 0.5f, teeZ1 + 0.3f,
                     0.0f, 2.0f,
                     rockC[0], rockC[1], rockC[2], 11u);
    addRockFormation(mb,
                     teeXMax + 0.5f, teeZ0 - 0.5f,
                     teeXMax + 0.5f, teeZ1 + 0.3f,
                     0.0f, 2.0f,
                     rockC[0]*1.05f, rockC[1]*1.05f, rockC[2]*1.05f, 22u);
    addRockFormation(mb,
                     teeXMin - 0.5f, teeZ0 - 0.5f,
                     teeXMax + 0.5f, teeZ0 - 0.5f,
                     0.0f, 1.8f,
                     rockC[0]*0.95f, rockC[1]*0.95f, rockC[2]*0.95f, 33u);
    // Front-edge stubs left and right of the ramp mouth (so the fairway is
    // visually closed on the forward side except where the ramp begins).
    addRockFormation(mb,
                     teeXMin - 0.5f, teeZ1 + 0.3f,
                     -rampW * 0.5f - 0.3f, teeZ1 + 0.3f,
                     0.0f, 1.8f,
                     rockC[0]*0.95f, rockC[1]*0.95f, rockC[2]*0.95f, 44u);
    addRockFormation(mb,
                     rampW * 0.5f + 0.3f, teeZ1 + 0.3f,
                     teeXMax + 0.5f, teeZ1 + 0.3f,
                     0.0f, 1.8f,
                     rockC[0]*0.95f, rockC[1]*0.95f, rockC[2]*0.95f, 55u);

    // ---- Approach ramp (tessellated, smoothly shaded) ----
    {
        const int RSEG = 18;
        float dz = rampZ1 - rampZ0;
        float dy = pedTopY - 0.0f;
        float len = std::sqrt(dz*dz + dy*dy);
        float ny =  dz / len;
        float nz = -dy / len;
        float nx =  0.0f;

        unsigned int prevL = 0, prevR = 0;
        bool havePrev = false;
        for (int i = 0; i <= RSEG; ++i) {
            float t = float(i) / RSEG;
            float z = rampZ0 + dz * t;
            float y = 0.0f   + dy * t;
            unsigned int l = mb.addVertex(-rampW*0.5f, y + 0.05f, z, nx, ny, nz,
                                          turfA[0], turfA[1], turfA[2]);
            unsigned int r = mb.addVertex( rampW*0.5f, y + 0.05f, z, nx, ny, nz,
                                           turfA[0], turfA[1], turfA[2]);
            // CCW from above (looking down -Y onto the ramp surface)
            if (havePrev) mb.addQuad(prevL, l, r, prevR);
            prevL = l; prevR = r;
            havePrev = true;
        }

        // Side rails along the ramp - tessellated wall (height varies with z)
        const int RW = 8;
        for (int i = 0; i < RW; ++i) {
            float t0 = float(i) / RW;
            float t1 = float(i + 1) / RW;
            float z0 = rampZ0 + dz * t0;
            float z1 = rampZ0 + dz * t1;
            float y0 = pedTopY * t0;
            float y1 = pedTopY * t1;
            // left rail
            addWallSeg(mb,
                       -rampW*0.5f, z0,
                       -rampW*0.5f, z1,
                       0.0f, y0 + 1.2f, 0.55f,
                       rockC[0], rockC[1], rockC[2]);
            // bump it down to the ramp top by reusing wall height that
            // tracks the slope - actually wallSeg uses a constant height,
            // so use stepped walls per segment
            addWallSeg(mb,
                        rampW*0.5f, z0,
                        rampW*0.5f, z1,
                       0.0f, y0 + 1.2f, 0.55f,
                       rockC[0], rockC[1], rockC[2]);
            (void)y1;
        }

        // Ramp underside (basalt skirt)
        addFlatQuad(mb,
                    -rampW*0.5f, 0.0f,    rampZ0,
                    -rampW*0.5f, pedTopY, rampZ1,
                     rampW*0.5f, pedTopY, rampZ1,
                     rampW*0.5f, 0.0f,    rampZ0,
                    rockC[0]*0.6f, rockC[1]*0.6f, rockC[2]*0.6f);
    }

    // ---- Lava moat ring around pedestal ----
    addRing(mb, 0.0f, 0.20f, pedZ, pedR + 0.5f, pedR + 5.0f, 60,
            lavaBright[0], lavaBright[1], lavaBright[2]);
    // darker inner ring of the moat for depth
    addRing(mb, 0.0f, 0.10f, pedZ, pedR + 0.3f, pedR + 0.55f, 60,
            lavaDark[0], lavaDark[1], lavaDark[2]);

    // ---- Radial lava channels flowing outward ----
    // 8 strips at evenly-spaced angles, each ~6 units long, pointing outward
    {
        const int N_CHANNELS = 8;
        const float TWO_PI = 6.28318530718f;
        const float rA = pedR + 5.0f;
        const float rB = pedR + 12.0f;
        const float chanHalfW = 0.6f;
        for (int i = 0; i < N_CHANNELS; ++i) {
            float a = float(i) / N_CHANNELS * TWO_PI + 0.18f;
            float ca = std::cos(a), sa = std::sin(a);
            // perpendicular for width
            float px = -sa, pz = ca;
            float ax = ca * rA + px * chanHalfW;
            float az = pedZ + sa * rA + pz * chanHalfW;
            float bx = ca * rA - px * chanHalfW;
            float bz = pedZ + sa * rA - pz * chanHalfW;
            float cx = ca * rB - px * chanHalfW * 0.4f; // taper to a tip
            float cz = pedZ + sa * rB - pz * chanHalfW * 0.4f;
            float dx = ca * rB + px * chanHalfW * 0.4f;
            float dz = pedZ + sa * rB + pz * chanHalfW * 0.4f;
            addFlatQuad(mb,
                        ax, 0.15f, az,
                        bx, 0.15f, bz,
                        cx, 0.15f, cz,
                        dx, 0.15f, dz,
                        lavaBright[0], lavaBright[1] * 0.95f, lavaBright[2]);
        }
    }

    // ---- Pedestal cylinder (smooth-shaded basalt + green top) ----
    addCylinder(mb,
                0.0f, pedZ,
                pedR,
                0.0f, pedTopY,
                64,                       // lots of segments for clean curve
                rockC[0], rockC[1], rockC[2],
                turfB[0], turfB[1], turfB[2]);

    // ---- Rim wall around the green's edge ----
    {
        const int RIM = 36;
        const float TWO_PI = 6.28318530718f;
        const float rimR = pedR - 0.30f;
        for (int i = 0; i < RIM; ++i) {
            float a0 = float(i) / RIM * TWO_PI;
            float a1 = float(i + 1) / RIM * TWO_PI;
            float x0 = std::cos(a0) * rimR;
            float z0 = pedZ + std::sin(a0) * rimR;
            float x1 = std::cos(a1) * rimR;
            float z1 = pedZ + std::sin(a1) * rimR;
            addWallSeg(mb, x0, z0, x1, z1, pedTopY, 0.6f, 0.35f,
                       rockC[0]*1.15f, rockC[1]*1.15f, rockC[2]*1.15f);
        }
    }

    // ---- Boulders dotted around the moat exterior (visual interest) ----
    {
        const float TWO_PI = 6.28318530718f;
        for (int i = 0; i < 7; ++i) {
            float a = (float(i) / 7) * TWO_PI + 0.55f;
            float r = pedR + 6.5f + prand(900u + i, 0) * 1.2f;
            float x = std::cos(a) * r;
            float z = pedZ + std::sin(a) * r;
            float br = 0.9f + prand(900u + i, 1) * 0.7f;
            addBoulder(mb, x, br * 0.55f, z, br,
                       rockC[0], rockC[1], rockC[2],
                       800u + (unsigned)i);
        }
    }

    // ---- Concrete tee pad ----
    addBox(mb, -2.5f, 0.0f, -3.5f, 5.0f, 0.25f, 3.0f,
           concrete[0], concrete[1], concrete[2]);

    // ---- Cup + flag dead centre on the pedestal ----
    _cupPos = glm::vec3(0.0f, pedTopY + 0.08f, pedZ);
    addDisc(mb, _cupPos.x, _cupPos.y, _cupPos.z, 0.45f, 20,
            0.02f, 0.02f, 0.02f);
    addFlag(mb, _cupPos.x, _cupPos.y, _cupPos.z, 7.0f);

    _teePos = glm::vec3(0.0f, 0.30f, -1.5f);
    _sceneCenter = glm::vec3(0.0f, 3.0f, 35.0f);
    _sceneRadius = 50.0f;

    _indexCount = uploadMesh(mb, _vao, _vbo, _ebo);
    std::cout << "[Hole14] built " << mb.vCount << " verts, "
              << mb.iCount << " indices" << std::endl;
}

void Hole14::draw(const glm::mat4& view,
                  const glm::mat4& proj,
                  const glm::vec3& lightDir,
                  const glm::vec3& viewPos) const
{
    if (_program == 0 || _vao == 0) return;
    glUseProgram(_program);
    glm::mat4 model(1.0f);
    glm::mat4 mvp = proj * view * model;
    glm::mat3 nrm = glm::mat3(1.0f);

    if (_uMVP        >= 0) glUniformMatrix4fv(_uMVP, 1, GL_FALSE, glm::value_ptr(mvp));
    if (_uModel      >= 0) glUniformMatrix4fv(_uModel, 1, GL_FALSE, glm::value_ptr(model));
    if (_uNormalMat  >= 0) glUniformMatrix3fv(_uNormalMat, 1, GL_FALSE, glm::value_ptr(nrm));
    if (_uLightDir   >= 0) glUniform3fv(_uLightDir, 1, glm::value_ptr(lightDir));
    if (_uLightColor >= 0) glUniform3f(_uLightColor, 1.0f, 0.97f, 0.9f);
    if (_uAmbient    >= 0) glUniform3f(_uAmbient, 0.30f, 0.30f, 0.35f);
    if (_uViewPos    >= 0) glUniform3fv(_uViewPos, 1, glm::value_ptr(viewPos));

    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Hole14::cleanup()
{
    if (_ebo) { glDeleteBuffers(1, &_ebo); _ebo = 0; }
    if (_vbo) { glDeleteBuffers(1, &_vbo); _vbo = 0; }
    if (_vao) { glDeleteVertexArrays(1, &_vao); _vao = 0; }
    _indexCount = 0;
}