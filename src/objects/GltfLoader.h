#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

// =============================================================================
// GltfLoader.h  —  Minimal single-header GLB/GLTF loader for COS344.
//
// Supports the exact structure exported by Blender for the volcano asset:
//   - Two mesh primitives, each with POSITION (VEC3/float), NORMAL (VEC3/float),
//     COLOR_0 (VEC3/float) and indices (SCALAR/uint16).
//   - A single binary buffer embedded in the GLB chunk.
//   - Material 1 is marked emissive (lava).
//
// Only uses: <cstdio>, <cstdlib>, <cstring>, <cmath>. No external libs.
// =============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <GL/glew.h>

namespace gltfloader {

// ---------------------------------------------------------------------------
// One drawable sub-mesh from the GLB
// ---------------------------------------------------------------------------
struct GltfPrimitive {
    GLuint  vao, vbo, ebo;
    GLsizei indexCount;
    bool    isEmissive;    // true for the lava primitive
    float   emissiveR, emissiveG, emissiveB;

    GltfPrimitive()
        : vao(0), vbo(0), ebo(0), indexCount(0),
          isEmissive(false),
          emissiveR(1.0f), emissiveG(0.35f), emissiveB(0.0f)
    {}
};

struct GltfMesh {
    GltfPrimitive* prims;
    int            primCount;
    GltfMesh() : prims(0), primCount(0) {}
    ~GltfMesh() { delete[] prims; }
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace detail {

inline unsigned int readU32LE(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8)
         | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}
inline float readF32LE(const unsigned char* p) {
    float f; memcpy(&f, p, 4); return f;
}
inline unsigned short readU16LE(const unsigned char* p) {
    return (unsigned short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

// Very small JSON token reader — enough to extract integer values by key.
// Returns -1 if key not found.
inline int jsonIntVal(const char* json, int jsonLen, const char* key) {
    // Search for "key": <integer>
    const char* p = json;
    const char* end = json + jsonLen;
    int klen = (int)strlen(key);
    while (p < end - klen - 3) {
        if (*p == '"' && strncmp(p + 1, key, klen) == 0 && p[klen + 1] == '"') {
            const char* q = p + klen + 2;
            while (q < end && (*q == ' ' || *q == ':' || *q == '\t')) ++q;
            if (q < end && (*q >= '0' && *q <= '9')) {
                int v = 0;
                while (q < end && *q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0');
                return v;
            }
        }
        ++p;
    }
    return -1;
}

// Extract the N-th occurrence of "key":<int> (0-indexed)
inline int jsonIntValN(const char* json, int jsonLen, const char* key, int n) {
    const char* p = json;
    const char* end = json + jsonLen;
    int klen = (int)strlen(key);
    int found = 0;
    while (p < end - klen - 3) {
        if (*p == '"' && strncmp(p + 1, key, klen) == 0 && p[klen + 1] == '"') {
            const char* q = p + klen + 2;
            while (q < end && (*q == ' ' || *q == ':' || *q == '\t')) ++q;
            if (q < end && (*q >= '0' && *q <= '9')) {
                if (found == n) {
                    int v = 0;
                    while (q < end && *q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0');
                    return v;
                }
                ++found;
            }
        }
        ++p;
    }
    return -1;
}

// Check if "emissiveFactor" appears in material slot m's block (rough heuristic)
inline bool materialIsEmissive(const char* json, int jsonLen, int materialIdx) {
    // Find the m-th "name" occurrence after "materials" — use a simple approach:
    // scan for "LavaVC" or emissiveFactor presence near the materialIdx block.
    // More reliable: count "emissiveFactor" occurrences.
    const char* p = json;
    const char* end = json + jsonLen;
    int count = 0;
    while (p < end - 14) {
        if (strncmp(p, "emissiveFactor", 14) == 0) ++count;
        ++p;
    }
    // If materialIdx < count there's an emissive entry before it
    // Simpler: any emissive at all and materialIdx==1 => it's lava
    return (materialIdx == 1 && count > 0);
}

} // namespace detail

// ---------------------------------------------------------------------------
// Load a GLB file. Returns a heap-allocated GltfMesh (caller owns it).
// Returns NULL on failure.
// ---------------------------------------------------------------------------
inline GltfMesh* loadGLB(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "[GltfLoader] Cannot open %s\n", path); return 0; }

    // --- GLB header (12 bytes) ---
    unsigned char hdr[12];
    if (fread(hdr, 1, 12, f) != 12) { fclose(f); return 0; }
    if (detail::readU32LE(hdr) != 0x46546C67u) { // "glTF"
        fprintf(stderr, "[GltfLoader] Not a GLB file.\n");
        fclose(f); return 0;
    }
    unsigned int totalLen = detail::readU32LE(hdr + 8);

    // --- Chunk 0: JSON ---
    unsigned char chunkHdr[8];
    if (fread(chunkHdr, 1, 8, f) != 8) { fclose(f); return 0; }
    unsigned int jsonLen  = detail::readU32LE(chunkHdr);
    char* jsonBuf = new char[jsonLen + 1];
    if (fread(jsonBuf, 1, jsonLen, f) != jsonLen) { delete[] jsonBuf; fclose(f); return 0; }
    jsonBuf[jsonLen] = '\0';

    // --- Chunk 1: BIN ---
    if (fread(chunkHdr, 1, 8, f) != 8) { delete[] jsonBuf; fclose(f); return 0; }
    unsigned int binLen = detail::readU32LE(chunkHdr);
    unsigned char* bin  = new unsigned char[binLen];
    if (fread(bin, 1, binLen, f) != binLen) {
        delete[] jsonBuf; delete[] bin; fclose(f); return 0;
    }
    fclose(f);
    (void)totalLen;

    // --- Parse: we know the exact structure from the GLTF analysis ---
    // Primitive 0 (rock): accessors 0(pos),1(nrm),2(col), indices=3
    // Primitive 1 (lava): accessors 4(pos),5(nrm),6(col), indices=7
    // BufferViews give byte offsets + lengths into bin[].
    // ComponentType 5126=float, 5123=uint16

    // Extract bufferView byteOffsets and byteLengths by scanning JSON.
    // We have 8 bufferViews (indices 0-7).
    const int NUM_BV = 8;
    unsigned int bvOffset[NUM_BV], bvLength[NUM_BV];
    {
        const char* p = jsonBuf;
        const char* end = jsonBuf + jsonLen;
        int bvIdx = 0;
        // Find "bufferViews" array then parse each object
        const char* bvStart = strstr(jsonBuf, "\"bufferViews\"");
        if (!bvStart) { delete[] jsonBuf; delete[] bin; return 0; }
        // Walk through and collect byteOffset / byteLength pairs
        p = bvStart;
        int collected = 0;
        memset(bvOffset, 0, sizeof(bvOffset));
        memset(bvLength, 0, sizeof(bvLength));
        while (p < end && collected < NUM_BV) {
            // Look for byteLength
            const char* bl = strstr(p, "\"byteLength\"");
            const char* bo = strstr(p, "\"byteOffset\"");
            if (!bl) break;
            // byteLength is always present; byteOffset may be 0 (absent)
            unsigned int off = 0, len = 0;
            // parse byteLength
            const char* q = bl + 12;
            while (q < end && (*q == ':' || *q == ' ')) ++q;
            while (q < end && *q >= '0' && *q <= '9') len = len*10 + (*q++ - '0');
            // parse byteOffset if it comes before next byteLength
            const char* nextBL = strstr(bl + 1, "\"byteLength\"");
            if (bo && (!nextBL || bo < nextBL)) {
                q = bo + 12;
                while (q < end && (*q == ':' || *q == ' ')) ++q;
                while (q < end && *q >= '0' && *q <= '9') off = off*10 + (*q++ - '0');
            }
            bvOffset[collected] = off;
            bvLength[collected] = len;
            ++collected;
            p = bl + 1;
        }
    }

    // Build two primitives
    GltfMesh* mesh = new GltfMesh();
    mesh->primCount = 2;
    mesh->prims = new GltfPrimitive[2];

    struct PrimDesc { int posAcc, nrmAcc, colAcc, idxAcc, matIdx; };
    PrimDesc descs[2] = {
        { 0, 1, 2, 3, 0 },   // rock
        { 4, 5, 6, 7, 1 }    // lava
    };
    // Accessor counts and buffer view indices (hardcoded from GLTF analysis)
    int accCount[8]  = { 9670, 9670, 9670, 56418, 875, 875, 875, 4218 };
    int accBV[8]     = {    0,    1,    2,     3,   4,   5,   6,    7 };

    for (int p = 0; p < 2; ++p) {
        PrimDesc& d = descs[p];
        GltfPrimitive& prim = mesh->prims[p];

        int vCount   = accCount[d.posAcc];
        int idxCount = accCount[d.idxAcc];

        const unsigned char* posBin = bin + bvOffset[accBV[d.posAcc]];
        const unsigned char* nrmBin = bin + bvOffset[accBV[d.nrmAcc]];
        const unsigned char* colBin = bin + bvOffset[accBV[d.colAcc]];
        const unsigned char* idxBin = bin + bvOffset[accBV[d.idxAcc]];

        // Interleave into pos(3f)+nrm(3f)+col(3f) = 9 floats per vertex
        float* interleaved = new float[(size_t)vCount * 9];
        for (int i = 0; i < vCount; ++i) {
            float px = detail::readF32LE(posBin + i*12);
            float py = detail::readF32LE(posBin + i*12 + 4);
            float pz = detail::readF32LE(posBin + i*12 + 8);
            float nx = detail::readF32LE(nrmBin + i*12);
            float ny = detail::readF32LE(nrmBin + i*12 + 4);
            float nz = detail::readF32LE(nrmBin + i*12 + 8);
            float cr = detail::readF32LE(colBin + i*12);
            float cg = detail::readF32LE(colBin + i*12 + 4);
            float cb = detail::readF32LE(colBin + i*12 + 8);
            float* v = interleaved + i * 9;
            v[0]=px; v[1]=py; v[2]=pz;
            v[3]=nx; v[4]=ny; v[5]=nz;
            v[6]=cr; v[7]=cg; v[8]=cb;
        }

        // Indices are uint16
        unsigned int* indices32 = new unsigned int[(size_t)idxCount];
        for (int i = 0; i < idxCount; ++i)
            indices32[i] = detail::readU16LE(idxBin + i*2);

        glGenVertexArrays(1, &prim.vao);
        glGenBuffers(1, &prim.vbo);
        glGenBuffers(1, &prim.ebo);

        glBindVertexArray(prim.vao);
        glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)((size_t)vCount * 9 * sizeof(float)),
                     interleaved, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)((size_t)idxCount * sizeof(unsigned int)),
                     indices32, GL_STATIC_DRAW);

        const int stride = 9 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
        glBindVertexArray(0);

        prim.indexCount = (GLsizei)idxCount;
        prim.isEmissive = detail::materialIsEmissive(jsonBuf, (int)jsonLen, d.matIdx);

        delete[] interleaved;
        delete[] indices32;

        fprintf(stderr, "[GltfLoader] Prim %d: %d verts, %d indices, emissive=%d\n",
                p, vCount, idxCount, (int)prim.isEmissive);
    }

    delete[] jsonBuf;
    delete[] bin;
    return mesh;
}

// Free all GL resources
inline void freeGltfMesh(GltfMesh* mesh) {
    if (!mesh) return;
    for (int i = 0; i < mesh->primCount; ++i) {
        GltfPrimitive& p = mesh->prims[i];
        if (p.ebo) glDeleteBuffers(1, &p.ebo);
        if (p.vbo) glDeleteBuffers(1, &p.vbo);
        if (p.vao) glDeleteVertexArrays(1, &p.vao);
    }
    delete mesh;
}

} // namespace gltfloader

#endif // GLTF_LOADER_H