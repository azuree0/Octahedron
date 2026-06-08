// 8-face regular octahedron; order-6 barycentric micro-triangles per face.

#pragma once

#include "octa_state.h"
#include "math3.h"

#include <array>

struct MeshTriangle {
    std::array<Vec3, 3> pos{};
    Vec3 normal{};
};

struct OctaMesh {
    std::array<Vec3, 6> corners{};
    std::array<std::array<int, 3>, kOctaFaces> faceCorners{};
    std::array<Vec3, kStickerCount> centroids{};
    std::array<MeshTriangle, kStickerCount> triangles{};
};

// Geometry: build mesh with halfExtent scaling (vertices at axis tips).
OctaMesh buildOctaMesh(float halfExtent);

// Geometry: sticker centroid in mesh space (matches octa_state slot order).
Vec3 stickerSlotCenterInMesh(float halfExtent, int slot);

// Row-major micro-triangle index on one face (must match OctaState::idx).
inline int meshStickerSlot(OctaFace face, int row, int col) {
    return static_cast<int>(face) * kTrisPerFace + row * kLatticeOrder + col;
}
