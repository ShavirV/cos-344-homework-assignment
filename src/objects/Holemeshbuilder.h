#ifndef HOLE_MESHBUILDER_H
#define HOLE_MESHBUILDER_H

// =============================================================================
// HoleMeshBuilder.h  –  Improved for COS344 Practical (Dino Park Hole 13)
// Header-only geometry helpers. Spec-allowed headers only: <cmath>, glm.
// =============================================================================

#include <cmath>
#include <glm/glm.hpp>

namespace holegeom {

// One interleaved attribute set: pos[3], normal[3], colour[3].
struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float cr, cg, cb;
};

class MeshBuilder {
public:
    Vertex*       verts;
    unsigned int* indices;
    unsigned int  vCount, vCap;
    unsigned int  iCount, iCap;

    MeshBuilder() : verts(0), indices(0),
                    vCount(0), vCap(0), iCount(0), iCap(0) {}
    ~MeshBuilder() { delete[] verts; delete[] indices; }

    void ensureVerts(unsigned int extra) {
        if (vCount + extra <= vCap) return;
        unsigned int nc = (vCap == 0) ? 512 : vCap * 2;
        while (nc < vCount + extra) nc *= 2;
        Vertex* nv = new Vertex[nc];
        for (unsigned int i = 0; i < vCount; ++i) nv[i] = verts[i];
        delete[] verts; verts = nv; vCap = nc;
    }
    void ensureIndices(unsigned int extra) {
        if (iCount + extra <= iCap) return;
        unsigned int nc = (iCap == 0) ? 1024 : iCap * 2;
        while (nc < iCount + extra) nc *= 2;
        unsigned int* ni = new unsigned int[nc];
        for (unsigned int i = 0; i < iCount; ++i) ni[i] = indices[i];
        delete[] indices; indices = ni; iCap = nc;
    }

    unsigned int addVertex(float x, float y, float z,
                           float nx_, float ny_, float nz_,
                           float r, float g, float b) {
        ensureVerts(1);
        Vertex& v = verts[vCount];
        v.px = x; v.py = y; v.pz = z;
        v.nx = nx_; v.ny = ny_; v.nz = nz_;
        v.cr = r; v.cg = g; v.cb = b;
        return vCount++;
    }

    void addTriangle(unsigned int a, unsigned int b, unsigned int c) {
        ensureIndices(3);
        indices[iCount++] = a;
        indices[iCount++] = b;
        indices[iCount++] = c;
    }

    void addQuad(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
        addTriangle(a, b, c);
        addTriangle(a, c, d);
    }
};

// ---- math helpers ----

inline void faceNormal(float ax, float ay, float az,
                       float bx, float by, float bz,
                       float cx, float cy, float cz,
                       float& nx, float& ny, float& nz)
{
    float ux = bx-ax, uy = by-ay, uz = bz-az;
    float vx = cx-ax, vy = cy-ay, vz = cz-az;
    nx = uy*vz - uz*vy;
    ny = uz*vx - ux*vz;
    nz = ux*vy - uy*vx;
    float len = std::sqrt(nx*nx + ny*ny + nz*nz);
    if (len > 1e-6f) { nx /= len; ny /= len; nz /= len; }
}

// Deterministic hash-based random in [0,1)
inline float prand(unsigned int seed, int k) {
    unsigned int v = seed * 1664525u + (unsigned int)k * 1013904223u;
    v ^= (v >> 13); v *= 0x5bd1e995u; v ^= (v >> 15);
    return (v & 0xffffffu) / float(0x1000000u);
}

// ---- primitive emitters ----

inline void addFlatQuad(MeshBuilder& mb,
                        float ax, float ay, float az,
                        float bx, float by, float bz,
                        float cx, float cy, float cz,
                        float dx, float dy, float dz,
                        float r, float g, float b)
{
    float nx, ny, nz;
    faceNormal(ax,ay,az, bx,by,bz, cx,cy,cz, nx,ny,nz);
    unsigned int i0 = mb.addVertex(ax,ay,az, nx,ny,nz, r,g,b);
    unsigned int i1 = mb.addVertex(bx,by,bz, nx,ny,nz, r,g,b);
    unsigned int i2 = mb.addVertex(cx,cy,cz, nx,ny,nz, r,g,b);
    unsigned int i3 = mb.addVertex(dx,dy,dz, nx,ny,nz, r,g,b);
    mb.addQuad(i0,i1,i2,i3);
}

inline void addFlatTri(MeshBuilder& mb,
                       float ax, float ay, float az,
                       float bx, float by, float bz,
                       float cx, float cy, float cz,
                       float r, float g, float b)
{
    float nx, ny, nz;
    faceNormal(ax,ay,az, bx,by,bz, cx,cy,cz, nx,ny,nz);
    unsigned int i0 = mb.addVertex(ax,ay,az, nx,ny,nz, r,g,b);
    unsigned int i1 = mb.addVertex(bx,by,bz, nx,ny,nz, r,g,b);
    unsigned int i2 = mb.addVertex(cx,cy,cz, nx,ny,nz, r,g,b);
    mb.addTriangle(i0,i1,i2);
}

// Axis-aligned box
inline void addBox(MeshBuilder& mb,
                   float ox, float oy, float oz,
                   float sx, float sy, float sz,
                   float r, float g, float b)
{
    float x0=ox, x1=ox+sx;
    float y0=oy, y1=oy+sy;
    float z0=oz, z1=oz+sz;
    // top (+Y)
    addFlatQuad(mb, x0,y1,z1, x1,y1,z1, x1,y1,z0, x0,y1,z0, r,g,b);
    // bottom (-Y)
    addFlatQuad(mb, x0,y0,z0, x1,y0,z0, x1,y0,z1, x0,y0,z1,
                r*0.55f, g*0.55f, b*0.55f);
    // front (-Z)
    addFlatQuad(mb, x0,y0,z0, x0,y1,z0, x1,y1,z0, x1,y0,z0,
                r*0.88f, g*0.88f, b*0.88f);
    // back (+Z)
    addFlatQuad(mb, x1,y0,z1, x1,y1,z1, x0,y1,z1, x0,y0,z1,
                r*0.88f, g*0.88f, b*0.88f);
    // left (-X)
    addFlatQuad(mb, x0,y0,z1, x0,y1,z1, x0,y1,z0, x0,y0,z0,
                r*0.82f, g*0.82f, b*0.82f);
    // right (+X)
    addFlatQuad(mb, x1,y0,z0, x1,y1,z0, x1,y1,z1, x1,y0,z1,
                r*0.82f, g*0.82f, b*0.82f);
}

// Vertical wall segment: from (x0,z0) to (x1,z1), height h above yBase, thickness t
inline void addWallSeg(MeshBuilder& mb,
                       float x0, float z0, float x1, float z1,
                       float yBase, float h, float t,
                       float r, float g, float b)
{
    float dx = x1-x0, dz = z1-z0;
    float L = std::sqrt(dx*dx + dz*dz);
    if (L < 1e-6f) return;
    dx /= L; dz /= L;
    float px2 = -dz, pz2 = dx;
    float hx = px2*(t*0.5f), hz = pz2*(t*0.5f);

    float ax = x0-hx, az = z0-hz;
    float bx = x0+hx, bz = z0+hz;
    float cx = x1+hx, cz = z1+hz;
    float dx2= x1-hx, dz2= z1-hz;
    float y0b = yBase, y1b = yBase+h;

    addFlatQuad(mb, ax,y1b,az, bx,y1b,bz, cx,y1b,cz, dx2,y1b,dz2, r,g,b);
    addFlatQuad(mb, ax,y0b,az, dx2,y0b,dz2, dx2,y1b,dz2, ax,y1b,az,
                r*0.82f, g*0.82f, b*0.82f);
    addFlatQuad(mb, bx,y1b,bz, cx,y1b,cz, cx,y0b,cz, bx,y0b,bz,
                r*0.82f, g*0.82f, b*0.82f);
    addFlatQuad(mb, ax,y0b,az, ax,y1b,az, bx,y1b,bz, bx,y0b,bz,
                r*0.72f, g*0.72f, b*0.72f);
    addFlatQuad(mb, cx,y0b,cz, cx,y1b,cz, dx2,y1b,dz2, dx2,y0b,dz2,
                r*0.72f, g*0.72f, b*0.72f);
}

// Faceted boulder: stacked top+bottom pyramids with jittered equatorial ring
inline void addBoulder(MeshBuilder& mb,
                       float cx, float cy, float cz,
                       float radius,
                       float r, float g, float b,
                       unsigned int seed)
{
    const int SEG = 10;
    float topY = cy + radius*(0.7f + prand(seed,0)*0.6f);
    float botY = cy - radius*(0.5f + prand(seed,1)*0.3f);
    float apexX = cx + (prand(seed,2)-0.5f)*radius*0.4f;
    float apexZ = cz + (prand(seed,3)-0.5f)*radius*0.4f;
    float baseX = cx + (prand(seed,4)-0.5f)*radius*0.2f;
    float baseZ = cz + (prand(seed,5)-0.5f)*radius*0.2f;

    float* rx = new float[SEG];
    float* ry = new float[SEG];
    float* rz = new float[SEG];
    for (int i = 0; i < SEG; ++i) {
        float a = float(i)/SEG * 6.28318530718f;
        float rr = radius*(0.75f + prand(seed, i+10)*0.5f);
        rx[i] = cx + std::cos(a)*rr;
        ry[i] = cy + (prand(seed, i+100)-0.5f)*radius*0.35f;
        rz[i] = cz + std::sin(a)*rr;
    }
    for (int i = 0; i < SEG; ++i) {
        int j = (i+1)%SEG;
        addFlatTri(mb, apexX, topY, apexZ, rx[i],ry[i],rz[i], rx[j],ry[j],rz[j],
                   r, g, b);
        addFlatTri(mb, baseX, botY, baseZ, rx[j],ry[j],rz[j], rx[i],ry[i],rz[i],
                   r*0.72f, g*0.72f, b*0.72f);
    }
    delete[] rx; delete[] ry; delete[] rz;
}

// Jagged rock-formation strip
inline void addRockFormation(MeshBuilder& mb,
                             float x0, float z0, float x1, float z1,
                             float yBase, float avgHeight,
                             float r, float g, float b,
                             unsigned int seed)
{
    float dx = x1-x0, dz = z1-z0;
    float L = std::sqrt(dx*dx + dz*dz);
    if (L < 1e-6f) return;
    float tx = dx/L, tz = dz/L;
    float nxr = -tz, nzr = tx;

    int count = int(L/1.4f);
    if (count < 2) count = 2;

    for (int i = 0; i < count; ++i) {
        float t = (float(i)+0.5f)/count;
        float jit  = (prand(seed, i*3+0)-0.5f)*0.7f;
        float side = (prand(seed, i*3+1)-0.5f)*0.55f;
        float bx = x0 + tx*(L*t+jit) + nxr*side;
        float bz = z0 + tz*(L*t+jit) + nzr*side;
        float h  = avgHeight*(0.65f + prand(seed, i*3+2)*1.0f);
        float brad = 0.7f + prand(seed, i*7)*0.65f;
        float rr = r*(0.80f + prand(seed, i*5)*0.38f);
        float gg = g*(0.80f + prand(seed, i*5+1)*0.38f);
        float bb = b*(0.80f + prand(seed, i*5+2)*0.38f);
        addBoulder(mb, bx, yBase+h*0.4f, bz, brad, rr,gg,bb, seed*31u+(unsigned)i);
        // Taller spire every 3rd rock for silhouette
        if (i%3 == 0) {
            addBoulder(mb,
                       bx+(prand(seed,i*11)-0.5f)*0.45f,
                       yBase+h*0.95f,
                       bz+(prand(seed,i*13)-0.5f)*0.45f,
                       brad*0.58f,
                       rr*0.88f, gg*0.88f, bb*0.88f,
                       seed*17u+(unsigned)i);
        }
    }
}

// Smooth cylinder: sides + top cap
inline void addCylinder(MeshBuilder& mb,
                        float cx, float cz,
                        float radius,
                        float yBottom, float yTop,
                        int segments,
                        float sideR, float sideG, float sideB,
                        float topR,  float topG,  float topB)
{
    const float TWO_PI = 6.28318530718f;
    unsigned int* bRing = new unsigned int[segments+1];
    unsigned int* tRing = new unsigned int[segments+1];
    for (int i = 0; i <= segments; ++i) {
        float a = float(i)/segments*TWO_PI;
        float ca = std::cos(a), sa = std::sin(a);
        float x = cx+ca*radius, z = cz+sa*radius;
        bRing[i] = mb.addVertex(x,yBottom,z, ca,0.f,sa, sideR,sideG,sideB);
        tRing[i] = mb.addVertex(x,yTop,   z, ca,0.f,sa, sideR,sideG,sideB);
    }
    for (int i = 0; i < segments; ++i)
        mb.addQuad(tRing[i], tRing[i+1], bRing[i+1], bRing[i]);
    delete[] bRing; delete[] tRing;

    unsigned int cIdx = mb.addVertex(cx,yTop,cz, 0.f,1.f,0.f, topR,topG,topB);
    unsigned int prev = 0;
    for (int i = 0; i <= segments; ++i) {
        float a = float(i)/segments*TWO_PI;
        unsigned int idx = mb.addVertex(cx+std::cos(a)*radius, yTop, cz+std::sin(a)*radius,
                                        0.f,1.f,0.f, topR,topG,topB);
        if (i > 0) mb.addTriangle(cIdx, idx, prev);
        prev = idx;
    }
}

// ---- NEW: Truncated cone (for volcano shell) ----
// Generates smooth-shaded sides + optional top disc.
// rBottom > rTop for the typical cone shape.
inline void addTruncatedCone(MeshBuilder& mb,
                             float cx, float cz,
                             float rBottom, float rTop,
                             float yBottom, float yTop,
                             int segments,
                             float r, float g, float b,
                             bool addTopCap = false)
{
    const float TWO_PI = 6.28318530718f;
    // The slope angle gives the outward-tilted normal
    float height = yTop - yBottom;
    float slope  = (rBottom - rTop) / (height > 1e-6f ? height : 1.f);
    float nlen   = std::sqrt(1.f + slope*slope);

    unsigned int* bRing = new unsigned int[segments+1];
    unsigned int* tRing = new unsigned int[segments+1];
    for (int i = 0; i <= segments; ++i) {
        float a  = float(i)/segments*TWO_PI;
        float ca = std::cos(a), sa = std::sin(a);
        float nx2 = ca/nlen, ny2 = slope/nlen, nz2 = sa/nlen;
        // Darken base, brighten top slightly
        float fB = 0.70f, fT = 1.00f;
        bRing[i] = mb.addVertex(cx+ca*rBottom, yBottom, cz+sa*rBottom,
                                nx2,ny2,nz2,  r*fB, g*fB, b*fB);
        tRing[i] = mb.addVertex(cx+ca*rTop,    yTop,    cz+sa*rTop,
                                nx2,ny2,nz2,  r*fT, g*fT, b*fT);
    }
    for (int i = 0; i < segments; ++i)
        mb.addQuad(tRing[i], tRing[i+1], bRing[i+1], bRing[i]);
    delete[] bRing; delete[] tRing;

    if (addTopCap) {
        // Flat top disc (caldera rim)
        unsigned int cIdx = mb.addVertex(cx,yTop,cz, 0.f,1.f,0.f, r*0.5f, g*0.5f, b*0.5f);
        unsigned int prev = 0;
        for (int i = 0; i <= segments; ++i) {
            float a = float(i)/segments*TWO_PI;
            unsigned int idx = mb.addVertex(
                cx+std::cos(a)*rTop, yTop, cz+std::sin(a)*rTop,
                0.f,1.f,0.f, r*0.55f, g*0.55f, b*0.55f);
            if (i > 0) mb.addTriangle(cIdx, idx, prev);
            prev = idx;
        }
    }
}

// Flat annular ring (lava moat / water ring)
inline void addRing(MeshBuilder& mb,
                    float cx, float y, float cz,
                    float rInner, float rOuter,
                    int segments,
                    float r, float g, float b)
{
    const float TWO_PI = 6.28318530718f;
    unsigned int* inner = new unsigned int[segments+1];
    unsigned int* outer = new unsigned int[segments+1];
    for (int i = 0; i <= segments; ++i) {
        float a  = float(i)/segments*TWO_PI;
        float ca = std::cos(a), sa = std::sin(a);
        inner[i] = mb.addVertex(cx+ca*rInner, y, cz+sa*rInner, 0.f,1.f,0.f, r,g,b);
        outer[i] = mb.addVertex(cx+ca*rOuter, y, cz+sa*rOuter, 0.f,1.f,0.f, r,g,b);
    }
    for (int i = 0; i < segments; ++i)
        mb.addQuad(inner[i+1], outer[i+1], outer[i], inner[i]);
    delete[] inner; delete[] outer;
}

// Flat disc
inline void addDisc(MeshBuilder& mb, float cx, float y, float cz,
                    float radius, int segments,
                    float r, float g, float b)
{
    unsigned int cIdx = mb.addVertex(cx,y,cz, 0.f,1.f,0.f, r,g,b);
    unsigned int prev = 0;
    for (int i = 0; i <= segments; ++i) {
        float a = float(i)/segments*6.28318530718f;
        unsigned int idx = mb.addVertex(
            cx+std::cos(a)*radius, y, cz+std::sin(a)*radius,
            0.f,1.f,0.f, r,g,b);
        if (i > 0) mb.addTriangle(cIdx, idx, prev);
        prev = idx;
    }
}

// Tapered cylinder (flag pole, bone shaft)
inline void addTaperedCylinder(MeshBuilder& mb,
                               float cx, float cz,
                               float rBottom, float rTop,
                               float yBottom, float yTop,
                               int segments,
                               float r, float g, float b)
{
    const float TWO_PI = 6.28318530718f;
    float slope = (rBottom-rTop)/(yTop-yBottom > 1e-6f ? yTop-yBottom : 1.f);
    float nl = std::sqrt(1.f+slope*slope);

    unsigned int* bRing = new unsigned int[segments+1];
    unsigned int* tRing = new unsigned int[segments+1];
    for (int i = 0; i <= segments; ++i) {
        float a = float(i)/segments*TWO_PI;
        float ca = std::cos(a), sa = std::sin(a);
        float nx2 = ca/nl, ny2 = slope/nl, nz2 = sa/nl;
        bRing[i] = mb.addVertex(cx+ca*rBottom,yBottom,cz+sa*rBottom, nx2,ny2,nz2, r,g,b);
        tRing[i] = mb.addVertex(cx+ca*rTop,   yTop,   cz+sa*rTop,    nx2,ny2,nz2, r,g,b);
    }
    for (int i = 0; i < segments; ++i)
        mb.addQuad(tRing[i], tRing[i+1], bRing[i+1], bRing[i]);
    delete[] bRing; delete[] tRing;
}

// ---- NEW: addArchPortal ----
// A practical cave/tunnel entrance arch: two thick pillars + curved capstone row on top.
inline void addArchPortal(MeshBuilder& mb,
                          float cx, float yBase, float cz,
                          float halfWidth,    // half the span between pillar centres
                          float pillarH,      // height of the standing pillars
                          float capR,         // radius of the curved cap
                          float depth,        // how thick/deep the arch is (along Z)
                          int capSegs,        // number of segments in the curved cap
                          float r, float g, float b)
{
    float pw = 1.2f;   // pillar width
    float pd = depth;  // pillar depth

    // Left pillar
    addBox(mb,
           cx - halfWidth - pw*0.5f, yBase,        cz - pd*0.5f,
           pw, pillarH, pd,
           r*0.85f, g*0.85f, b*0.85f);
    // Right pillar
    addBox(mb,
           cx + halfWidth - pw*0.5f, yBase,        cz - pd*0.5f,
           pw, pillarH, pd,
           r*0.85f, g*0.85f, b*0.85f);

    // Curved cap: a row of boxes arranged in a semicircle above the span
    float arcY = yBase + pillarH;
    const float PI = 3.14159265f;
    for (int i = 0; i < capSegs; ++i) {
        float a0 = PI * float(i)   / capSegs;
        float a1 = PI * float(i+1) / capSegs;
        float aMid = (a0 + a1) * 0.5f;
        float x = cx + capR * std::cos(PI - aMid);   // -halfWidth to +halfWidth
        float y = arcY + capR * std::sin(aMid) - capR; // rises in the middle
        float blockW = capR * (a1 - a0) * 1.1f;
        float blockH = 1.2f;
        addBox(mb,
               x - blockW*0.5f, y, cz - pd*0.5f,
               blockW, blockH, pd,
               r, g, b);
    }
}

// Flag pole + two-sided flag triangle
inline void addFlag(MeshBuilder& mb,
                    float x, float yBase, float z,
                    float poleH)
{
    const float poleR = 0.12f;
    addTaperedCylinder(mb, x, z, poleR, poleR*0.70f,
                       yBase, yBase+poleH, 14,
                       0.90f, 0.90f, 0.90f);
    float top = yBase + poleH;
    float w   = poleH * 0.50f;
    // front
    addFlatTri(mb,
               x,     top,           z,
               x + w, top - w*0.28f, z,
               x,     top - w*0.68f, z,
               0.92f, 0.12f, 0.10f);
    // back
    addFlatTri(mb,
               x,     top,           z,
               x,     top - w*0.68f, z,
               x + w, top - w*0.28f, z,
               0.92f, 0.12f, 0.10f);
}

// Dinosaur bone obstacle
inline void addBoneObstacle(MeshBuilder& mb,
                            float x, float yBase, float z,
                            float length, float radius,
                            float rotY,
                            float r, float g, float b)
{
    float ca = std::cos(rotY), sa = std::sin(rotY);
    float ex = x + ca*(length*0.5f), ez = z + sa*(length*0.5f);
    float sx = x - ca*(length*0.5f), sz = z - sa*(length*0.5f);
    float cy = yBase + radius*1.1f;

    float perpX = -sa, perpZ = ca;
    float hw = radius*0.55f;
    float bx0 = sx+perpX*hw, bz0 = sz+perpZ*hw;
    float bx1 = sx-perpX*hw, bz1 = sz-perpZ*hw;
    float fx0 = ex+perpX*hw, fz0 = ez+perpZ*hw;
    float fx1 = ex-perpX*hw, fz1 = ez-perpZ*hw;
    float y0 = cy-radius*0.55f, y1 = cy+radius*0.55f;

    addFlatQuad(mb, bx0,y1,bz0, fx0,y1,fz0, fx1,y1,fz1, bx1,y1,bz1, r,g,b);
    addFlatQuad(mb, bx1,y0,bz1, fx1,y0,fz1, fx0,y0,fz0, bx0,y0,bz0, r*0.65f,g*0.65f,b*0.65f);
    addFlatQuad(mb, bx0,y0,bz0, fx0,y0,fz0, fx0,y1,fz0, bx0,y1,bz0, r*0.82f,g*0.82f,b*0.82f);
    addFlatQuad(mb, fx1,y0,fz1, bx1,y0,bz1, bx1,y1,bz1, fx1,y1,fz1, r*0.82f,g*0.82f,b*0.82f);
    addFlatQuad(mb, bx1,y0,bz1, bx0,y0,bz0, bx0,y1,bz0, bx1,y1,bz1, r*0.75f,g*0.75f,b*0.75f);
    addFlatQuad(mb, fx0,y0,fz0, fx1,y0,fz1, fx1,y1,fz1, fx0,y1,fz0, r*0.75f,g*0.75f,b*0.75f);

    addBoulder(mb, sx+perpX*radius*0.85f, cy, sz+perpZ*radius*0.85f, radius*0.95f, r,g,b, 7001u);
    addBoulder(mb, sx-perpX*radius*0.85f, cy, sz-perpZ*radius*0.85f, radius*0.95f, r,g,b, 7002u);
    addBoulder(mb, ex+perpX*radius*0.85f, cy, ez+perpZ*radius*0.85f, radius*0.95f, r,g,b, 7003u);
    addBoulder(mb, ex-perpX*radius*0.85f, cy, ez-perpZ*radius*0.85f, radius*0.95f, r,g,b, 7004u);
}

// ---- NEW: Stepped lava channel – a sunken trench along the foot of the volcano ----
// Rendered as a flat inner quad (lava) with angled side walls dropping down.
inline void addLavaTrench(MeshBuilder& mb,
                          float cx, float cy, float cz,
                          float outerR, float innerR,
                          float depth,
                          int segments,
                          float lavaR, float lavaG, float lavaB,
                          float wallR, float wallG, float wallB)
{
    const float TWO_PI = 6.28318530718f;
    // Lava floor ring (innermost, at cy - depth)
    addRing(mb, cx, cy-depth, cz, innerR*0.6f, innerR, segments, lavaR, lavaG, lavaB);

    // Angled wall: from outer rim (cx+outerR, cy) down to inner rim edge (cx+innerR, cy-depth)
    unsigned int* outerTop  = new unsigned int[segments+1];
    unsigned int* innerBot  = new unsigned int[segments+1];

    float wallDx = outerR - innerR;
    float wallDy = -depth;
    float wLen   = std::sqrt(wallDx*wallDx + wallDy*wallDy);
    float wny    = wallDx / wLen;  // normal component
    float wnr    = wallDy / wLen;

    for (int i = 0; i <= segments; ++i) {
        float a = float(i)/segments*TWO_PI;
        float ca = std::cos(a), sa = std::sin(a);
        // The outward normal for each face has radial + upward component
        float nx2 = ca*wny, ny2 = wnr, nz2 = sa*wny; // angled outward+up
        outerTop[i] = mb.addVertex(cx+ca*outerR, cy,       cz+sa*outerR,
                                   nx2,ny2,nz2,  wallR,wallG,wallB);
        innerBot[i] = mb.addVertex(cx+ca*innerR, cy-depth, cz+sa*innerR,
                                   nx2,ny2,nz2,  wallR*0.65f,wallG*0.65f,wallB*0.65f);
    }
    for (int i = 0; i < segments; ++i)
        mb.addQuad(outerTop[i], outerTop[i+1], innerBot[i+1], innerBot[i]);

    delete[] outerTop; delete[] innerBot;
}

// ---- NEW: Palm tree (simplified) ----
// Trunk = tall thin cylinder, fronds = 6 flat quads radiating from top.
inline void addPalmTree(MeshBuilder& mb,
                        float cx, float yBase, float cz,
                        float trunkH, float trunkR,
                        float frondLen)
{
    // Trunk: brown cylinder, slight taper
    addTaperedCylinder(mb, cx, cz, trunkR, trunkR*0.55f, yBase, yBase+trunkH, 10,
                       0.42f, 0.28f, 0.14f);

    // Fronds: 6 flat angled quads radiating from tip
    float tipY = yBase + trunkH;
    const int FRONDS = 6;
    for (int i = 0; i < FRONDS; ++i) {
        float a   = float(i)/FRONDS * 6.28318530718f;
        float ca  = std::cos(a), sa  = std::sin(a);
        // Frond droop: tip at trunkH*0.85, endpoint at trunkH*0.75
        float ex  = cx + ca*frondLen;
        float ey  = tipY - frondLen*0.35f;
        float ez  = cz + sa*frondLen;
        float hw  = frondLen*0.18f;  // half-width at base
        float ew  = frondLen*0.06f;  // half-width at tip
        float px2 = -sa, pz2 = ca;   // perpendicular

        // Two-sided flat quad for frond
        addFlatTri(mb,
                   cx + px2*hw, tipY,  cz + pz2*hw,
                   cx - px2*hw, tipY,  cz - pz2*hw,
                   ex + px2*ew, ey,    ez + pz2*ew,
                   0.18f, 0.55f, 0.15f);
        addFlatTri(mb,
                   cx - px2*hw, tipY,  cz - pz2*hw,
                   cx + px2*hw, tipY,  cz + pz2*hw,
                   ex + px2*ew, ey,    ez + pz2*ew,
                   0.18f, 0.55f, 0.15f);
        // Back face
        addFlatTri(mb,
                   cx - px2*hw, tipY,  cz - pz2*hw,
                   cx + px2*hw, tipY,  cz + pz2*hw,
                   ex - px2*ew, ey,    ez - pz2*ew,
                   0.14f, 0.48f, 0.12f);
        addFlatTri(mb,
                   cx + px2*hw, tipY,  cz + pz2*hw,
                   cx - px2*hw, tipY,  cz - pz2*hw,
                   ex - px2*ew, ey,    ez - pz2*ew,
                   0.14f, 0.48f, 0.12f);
    }
}

// ---- NEW: Stepping stones cluster ----
// Small flat discs scattered around a centre point.
inline void addSteppingStones(MeshBuilder& mb,
                              float cx, float y, float cz,
                              float spread, float stoneR,
                              int count, unsigned int seed,
                              float r, float g, float b)
{
    for (int i = 0; i < count; ++i) {
        float a   = prand(seed, i*3)   * 6.28318530718f;
        float rad = prand(seed, i*3+1) * spread;
        float sx  = cx + std::cos(a)*rad;
        float sz  = cz + std::sin(a)*rad;
        float sr  = stoneR * (0.6f + prand(seed, i*3+2)*0.8f);
        float fr  = r * (0.85f + prand(seed, i*7)  *0.3f);
        float fg  = g * (0.85f + prand(seed, i*7+1)*0.3f);
        float fb  = b * (0.85f + prand(seed, i*7+2)*0.3f);
        addDisc(mb, sx, y+0.06f, sz, sr, 8, fr,fg,fb);
        // Tiny raised edge for a 3D look
        addCylinder(mb, sx, sz, sr, y, y+0.07f, 8, fr*0.72f,fg*0.72f,fb*0.72f, fr,fg,fb);
    }
}

// ---- NEW: Wooden bridge ----
// Deck + railings + plank lines
inline void addWoodenBridge(MeshBuilder& mb,
                            float x0, float z0,
                            float x1, float z1,
                            float yBase, float width)
{
    float dx = x1-x0, dz = z1-z0;
    float L  = std::sqrt(dx*dx + dz*dz);
    if (L < 1e-6f) return;
    float tx = dx/L, tz = dz/L;
    float px2 = -tz, pz2 = tx;
    float hw = width*0.5f;

    // Deck
    float bx0 = x0+px2*hw, bz0 = z0+pz2*hw;
    float bx1 = x0-px2*hw, bz1 = z0-pz2*hw;
    float fx0 = x1+px2*hw, fz0 = z1+pz2*hw;
    float fx1 = x1-px2*hw, fz1 = z1-pz2*hw;
    addFlatQuad(mb,
                bx0, yBase+0.15f, bz0,
                fx0, yBase+0.15f, fz0,
                fx1, yBase+0.15f, fz1,
                bx1, yBase+0.15f, bz1,
                0.52f, 0.33f, 0.18f);
    // Deck side boards
    addBox(mb, x0+tx*0.f-px2*(hw+0.12f), yBase, z0+tz*0.f-pz2*(hw+0.12f),
           L*0.98f, 0.35f, 0.18f,  0.42f, 0.26f, 0.14f);

    // Planks (thin strips across width)
    int nPlanks = int(L / 0.8f);
    for (int i = 0; i <= nPlanks; ++i) {
        float t2 = float(i)/nPlanks;
        float mx = x0+tx*L*t2, mz = z0+tz*L*t2;
        addBox(mb,
               mx+px2*(hw+0.02f)-px2*0.06f, yBase+0.14f, mz+pz2*(hw+0.02f)-pz2*0.06f,
               width+0.04f+0.06f, 0.07f, 0.12f,
               0.38f, 0.24f, 0.12f);
    }

    // Railing posts
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i <= 4; ++i) {
            float t2 = float(i)/4;
            float px3 = x0+tx*L*t2 + px2*side*(hw+0.08f);
            float pz3 = z0+tz*L*t2 + pz2*side*(hw+0.08f);
            addCylinder(mb, px3, pz3, 0.07f, yBase, yBase+0.90f, 6,
                        0.38f,0.24f,0.12f, 0.38f,0.24f,0.12f);
        }
        // Handrail
        float hx0 = x0+px2*side*(hw+0.08f), hz0 = z0+pz2*side*(hw+0.08f);
        float hx1 = x1+px2*side*(hw+0.08f), hz1 = z1+pz2*side*(hw+0.08f);
        addWallSeg(mb, hx0,hz0, hx1,hz1, yBase+0.82f, 0.10f, 0.10f,
                   0.38f,0.24f,0.12f);
    }
}

// Build VAO/VBO/EBO from MeshBuilder, return element count
inline GLsizei uploadMesh(const MeshBuilder& mb,
                          GLuint& vao, GLuint& vbo, GLuint& ebo)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(mb.vCount * sizeof(Vertex)),
                 mb.verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(mb.iCount * sizeof(unsigned int)),
                 mb.indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)(sizeof(float)*6));
    glBindVertexArray(0);
    return (GLsizei)mb.iCount;
}

} // namespace holegeom
#endif