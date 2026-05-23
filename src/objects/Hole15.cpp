#include "Hole15.h"
#include "HoleMeshBuilder.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace holegeom;

Hole15::Hole15()
    : _vao(0), _vbo(0), _ebo(0), _indexCount(0), _program(0),
      _uMVP(-1), _uModel(-1), _uNormalMat(-1),
      _uLightDir(-1), _uLightColor(-1), _uAmbient(-1), _uViewPos(-1),
      _teePos(0.0f), _cupPos(0.0f), _sceneCenter(0.0f), _sceneRadius(50.0f)
{}

Hole15::~Hole15() { cleanup(); }

void Hole15::init(GLuint shaderProgram)
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

void Hole15::build()
{
    MeshBuilder mb;

    // ---- Palette ----
    const float turf[3]    = { 0.10f, 0.55f, 0.18f };
    const float turfDark[3]= { 0.08f, 0.45f, 0.14f };
    const float rockC[3]   = { 0.20f, 0.18f, 0.18f };
    const float concrete[3]= { 0.72f, 0.70f, 0.66f };
    const float darkSand[3]= { 0.28f, 0.23f, 0.20f };

    // ---- Ramp parameters ----
    // The ramp spirals around the world-origin (which is where the volcano
    // Blender asset will sit later). 170 degrees of arc, rising from y=0 at
    // theta=THETA_START to y=10 at theta=THETA_END.
    const int   STEPS       = 80;
    const float THETA_START = -150.0f * 3.14159265f / 180.0f;
    const float THETA_END   =   20.0f * 3.14159265f / 180.0f;
    const float R_INNER     = 16.0f;          // closer to where volcano sits
    const float R_OUTER     = 26.0f;          // wide ramp (10 units across)
    const float Y_START     = 0.0f;
    const float Y_END       = 10.0f;

    // ---- Underlying ground pad (so camera doesn't see the void) ----
    // Wound CCW from above so the surface normal points +Y.
    addFlatQuad(mb,
                -40.0f, -0.05f, -40.0f,
                -40.0f, -0.05f,  40.0f,
                 40.0f, -0.05f,  40.0f,
                 40.0f, -0.05f, -40.0f,
                darkSand[0], darkSand[1], darkSand[2]);

    // ---- Ramp surface (smooth quad strip with proper normals) ----
    unsigned int* innerIdx = new unsigned int[STEPS + 1];
    unsigned int* outerIdx = new unsigned int[STEPS + 1];

    auto rampPoint = [&](int i, bool outer,
                         float& x, float& y, float& z) {
        float t = float(i) / STEPS;
        float theta = THETA_START + (THETA_END - THETA_START) * t;
        float r = outer ? R_OUTER : R_INNER;
        x = std::cos(theta) * r;
        z = std::sin(theta) * r;
        y = Y_START + (Y_END - Y_START) * t;
    };

    for (int i = 0; i <= STEPS; ++i) {
        float ix, iy, iz;  rampPoint(i, false, ix, iy, iz);
        float ox, oy, oz;  rampPoint(i, true,  ox, oy, oz);

        // Tangent: finite difference along arc (clamped at endpoints)
        int ia = (i == 0)     ? 0     : i - 1;
        int ib = (i == STEPS) ? STEPS : i + 1;
        float ax, ay, az; rampPoint(ia, false, ax, ay, az);
        float bx, by, bz; rampPoint(ib, false, bx, by, bz);
        float tx = bx - ax, ty = by - ay, tz = bz - az;

        // Radial (inner -> outer)
        float rx = ox - ix, ry = oy - iy, rz = oz - iz;

        // Surface normal = tangent x radial
        float nx = ty*rz - tz*ry;
        float ny = tz*rx - tx*rz;
        float nz = tx*ry - ty*rx;
        float L = std::sqrt(nx*nx + ny*ny + nz*nz);
        if (L > 1e-6f) { nx /= L; ny /= L; nz /= L; }
        if (ny < 0) { nx = -nx; ny = -ny; nz = -nz; }

        // Slight colour gradient along the climb - darker green as we ascend
        float tt = float(i) / STEPS;
        float r = turf[0] * (1.0f - tt) + turfDark[0] * tt;
        float g = turf[1] * (1.0f - tt) + turfDark[1] * tt;
        float b = turf[2] * (1.0f - tt) + turfDark[2] * tt;

        innerIdx[i] = mb.addVertex(ix, iy + 0.04f, iz, nx, ny, nz, r, g, b);
        outerIdx[i] = mb.addVertex(ox, oy + 0.04f, oz, nx, ny, nz, r, g, b);
    }
    for (int i = 0; i < STEPS; ++i) {
        // CCW from above (so the ramp top faces upward).
        mb.addQuad(innerIdx[i + 1], outerIdx[i + 1], outerIdx[i], innerIdx[i]);
    }

    // ---- Outer parapet wall (basalt) along the full ramp ----
    for (int i = 0; i < STEPS; ++i) {
        float t0 = float(i) / STEPS;
        float t1 = float(i + 1) / STEPS;
        float th0 = THETA_START + (THETA_END - THETA_START) * t0;
        float th1 = THETA_START + (THETA_END - THETA_START) * t1;
        float y0 = Y_START + (Y_END - Y_START) * t0;
        float y1 = Y_START + (Y_END - Y_START) * t1;
        const float wallH = 1.8f;
        const float rOut  = R_OUTER;
        const float rOutT = R_OUTER + 1.0f;
        float ax = std::cos(th0) * rOut,  az = std::sin(th0) * rOut;
        float bx = std::cos(th0) * rOutT, bz = std::sin(th0) * rOutT;
        float cx = std::cos(th1) * rOutT, cz = std::sin(th1) * rOutT;
        float dx = std::cos(th1) * rOut,  dz = std::sin(th1) * rOut;
        // top of parapet
        addFlatQuad(mb,
                    ax, y0 + wallH, az,
                    bx, y0 + wallH, bz,
                    cx, y1 + wallH, cz,
                    dx, y1 + wallH, dz,
                    rockC[0], rockC[1], rockC[2]);
        // outer face
        addFlatQuad(mb,
                    bx, y0,         bz,
                    cx, y1,         cz,
                    cx, y1 + wallH, cz,
                    bx, y0 + wallH, bz,
                    rockC[0]*0.85f, rockC[1]*0.85f, rockC[2]*0.85f);
        // inner face
        addFlatQuad(mb,
                    ax, y0 + wallH, az,
                    dx, y1 + wallH, dz,
                    dx, y1,         dz,
                    ax, y0,         az,
                    rockC[0]*0.95f, rockC[1]*0.95f, rockC[2]*0.95f);
    }

    // ---- Inner kerb wall along the volcano side (shorter, hugs the cone) ----
    for (int i = 0; i < STEPS; ++i) {
        float t0 = float(i) / STEPS;
        float t1 = float(i + 1) / STEPS;
        float th0 = THETA_START + (THETA_END - THETA_START) * t0;
        float th1 = THETA_START + (THETA_END - THETA_START) * t1;
        float y0 = Y_START + (Y_END - Y_START) * t0;
        float y1 = Y_START + (Y_END - Y_START) * t1;
        const float wallH = 0.9f;
        const float rIn  = R_INNER;
        const float rInT = R_INNER - 0.7f;
        float ax = std::cos(th0) * rIn,  az = std::sin(th0) * rIn;
        float bx = std::cos(th0) * rInT, bz = std::sin(th0) * rInT;
        float cx = std::cos(th1) * rInT, cz = std::sin(th1) * rInT;
        float dx = std::cos(th1) * rIn,  dz = std::sin(th1) * rIn;
        addFlatQuad(mb,
                    ax, y0 + wallH, az,
                    bx, y0 + wallH, bz,
                    cx, y1 + wallH, cz,
                    dx, y1 + wallH, dz,
                    rockC[0]*1.1f, rockC[1]*1.1f, rockC[2]*1.1f);
        addFlatQuad(mb,
                    ax, y0 + wallH, az,
                    dx, y1 + wallH, dz,
                    dx, y1,         dz,
                    ax, y0,         az,
                    rockC[0], rockC[1], rockC[2]);
        addFlatQuad(mb,
                    bx, y0,         bz,
                    cx, y1,         cz,
                    cx, y1 + wallH, cz,
                    bx, y0 + wallH, bz,
                    rockC[0]*0.9f, rockC[1]*0.9f, rockC[2]*0.9f);
    }

    // ---- Outer skirt: drop the outer rim to the ground so the ramp looks
    // solid from any angle.
    for (int i = 0; i < STEPS; ++i) {
        float t0 = float(i) / STEPS;
        float t1 = float(i + 1) / STEPS;
        float th0 = THETA_START + (THETA_END - THETA_START) * t0;
        float th1 = THETA_START + (THETA_END - THETA_START) * t1;
        float y0 = Y_START + (Y_END - Y_START) * t0;
        float y1 = Y_START + (Y_END - Y_START) * t1;
        float ox0 = std::cos(th0) * R_OUTER, oz0 = std::sin(th0) * R_OUTER;
        float ox1 = std::cos(th1) * R_OUTER, oz1 = std::sin(th1) * R_OUTER;
        addFlatQuad(mb,
                    ox0, 0.0f, oz0,
                    ox1, 0.0f, oz1,
                    ox1, y1,   oz1,
                    ox0, y0,   oz0,
                    rockC[0]*0.65f, rockC[1]*0.65f, rockC[2]*0.65f);
    }

    // ---- Inner skirt: drop the inner rim to ground too (volcano asset will
    // hide most of it but it's good to have for any vantage). ----
    for (int i = 0; i < STEPS; ++i) {
        float t0 = float(i) / STEPS;
        float t1 = float(i + 1) / STEPS;
        float th0 = THETA_START + (THETA_END - THETA_START) * t0;
        float th1 = THETA_START + (THETA_END - THETA_START) * t1;
        float y0 = Y_START + (Y_END - Y_START) * t0;
        float y1 = Y_START + (Y_END - Y_START) * t1;
        float ix0 = std::cos(th0) * R_INNER, iz0 = std::sin(th0) * R_INNER;
        float ix1 = std::cos(th1) * R_INNER, iz1 = std::sin(th1) * R_INNER;
        // Quad facing inward (toward volcano centre)
        addFlatQuad(mb,
                    ix1, 0.0f, iz1,
                    ix0, 0.0f, iz0,
                    ix0, y0,   iz0,
                    ix1, y1,   iz1,
                    rockC[0]*0.6f, rockC[1]*0.6f, rockC[2]*0.6f);
    }

    // ---- End caps at the start/end of the ramp arc ----
    {
        // Start cap (tee end)
        float th = THETA_START;
        float ca = std::cos(th), sa = std::sin(th);
        float yT = Y_START;
        addFlatQuad(mb,
                    ca * R_INNER, 0.0f, sa * R_INNER,
                    ca * R_OUTER, 0.0f, sa * R_OUTER,
                    ca * R_OUTER, yT,   sa * R_OUTER,
                    ca * R_INNER, yT,   sa * R_INNER,
                    rockC[0]*0.7f, rockC[1]*0.7f, rockC[2]*0.7f);
    }
    {
        // End cap (cup end)
        float th = THETA_END;
        float ca = std::cos(th), sa = std::sin(th);
        float yT = Y_END;
        addFlatQuad(mb,
                    ca * R_OUTER, 0.0f, sa * R_OUTER,
                    ca * R_INNER, 0.0f, sa * R_INNER,
                    ca * R_INNER, yT,   sa * R_INNER,
                    ca * R_OUTER, yT,   sa * R_OUTER,
                    rockC[0]*0.7f, rockC[1]*0.7f, rockC[2]*0.7f);
    }

    // ---- Boulders scattered along the ramp (obstacles) ----
    {
        const int N_BOULDERS = 6;
        for (int i = 0; i < N_BOULDERS; ++i) {
            float t = (float(i) + 0.5f) / N_BOULDERS;
            float theta = THETA_START + (THETA_END - THETA_START) * t;
            // Alternate which side of the ramp
            float side = (i % 2 == 0) ? 0.30f : 0.70f;
            float r = R_INNER + (R_OUTER - R_INNER) * side;
            float y = Y_START + (Y_END - Y_START) * t;
            float bx = std::cos(theta) * r;
            float bz = std::sin(theta) * r;
            float br = 0.9f + prand(444u + i, 0) * 0.5f;
            addBoulder(mb, bx, y + br * 0.5f, bz, br,
                       rockC[0], rockC[1], rockC[2],
                       500u + (unsigned)i);
        }
    }

    // ---- Tee pad at the start of the ramp ----
    {
        float th = THETA_START;
        float midR = (R_INNER + R_OUTER) * 0.5f;
        float tx = std::cos(th) * midR;
        float tz = std::sin(th) * midR;
        addBox(mb, tx - 2.5f, Y_START + 0.05f, tz - 2.5f,
               5.0f, 0.25f, 5.0f,
               concrete[0], concrete[1], concrete[2]);
        _teePos = glm::vec3(tx, Y_START + 0.30f, tz);
    }

    // ---- Cup + flag at the top of the ramp ----
    {
        float th = THETA_END;
        float midR = (R_INNER + R_OUTER) * 0.5f;
        float cx = std::cos(th) * midR;
        float cz = std::sin(th) * midR;
        float cy = Y_END + 0.10f;
        _cupPos = glm::vec3(cx, cy, cz);
        addDisc(mb, cx, cy, cz, 0.45f, 20, 0.02f, 0.02f, 0.02f);
        addFlag(mb, cx, cy, cz, 7.0f);
    }

    delete[] innerIdx;
    delete[] outerIdx;

    _sceneCenter = glm::vec3(0.0f, 4.0f, 0.0f);
    _sceneRadius = 45.0f;

    _indexCount = uploadMesh(mb, _vao, _vbo, _ebo);
    std::cout << "[Hole15] built " << mb.vCount << " verts, "
              << mb.iCount << " indices" << std::endl;
}

void Hole15::draw(const glm::mat4& view,
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

void Hole15::cleanup()
{
    if (_ebo) { glDeleteBuffers(1, &_ebo); _ebo = 0; }
    if (_vbo) { glDeleteBuffers(1, &_vbo); _vbo = 0; }
    if (_vao) { glDeleteVertexArrays(1, &_vao); _vao = 0; }
    _indexCount = 0;
}