// Cap-slab masks and 120-degree face-turn permutations (topology shared with octa_mesh).

#include "octa_state.h"

#include "octa_mesh.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

int nearestStickerSlot(Vec3 p, const std::array<Vec3, kStickerCount>& centroids) {
    int best = 0;
    float bestD2 = 1e30f;
    for (int i = 0; i < kStickerCount; ++i) {
        const Vec3 d = centroids[static_cast<std::size_t>(i)] - p;
        const float d2 = dot(d, d);
        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best;
}

} // namespace

Vec3 OctaState::faceOutwardNormal(OctaFace face) {
    static const std::array<Vec3, kOctaFaces> kNormals = {
        normalize({1.f, 1.f, 1.f}),
        normalize({1.f, 1.f, -1.f}),
        normalize({1.f, -1.f, -1.f}),
        normalize({1.f, -1.f, 1.f}),
        normalize({-1.f, 1.f, 1.f}),
        normalize({-1.f, 1.f, -1.f}),
        normalize({-1.f, -1.f, -1.f}),
        normalize({-1.f, -1.f, 1.f}),
    };
    return kNormals[static_cast<std::size_t>(face)];
}

OctaState::OctaState(const OctaMesh& mesh) {
    centroids_ = mesh.centroids;
    reset();
}

void OctaState::reset() {
    for (int i = 0; i < kStickerCount; ++i) {
        const int faceIdx = i / kTrisPerFace;
        color_[static_cast<std::size_t>(i)] = faceIdx % kOctaFaces;
    }
    history_.clear();
}

void OctaState::stickersInSlab(OctaFace face, int depth, const OctaMesh& mesh, std::vector<int>& outIndices) {
    outIndices.clear();
    const int d = std::clamp(depth, 1, kLatticeOrder);
    for (int i = 0; i < kStickerCount; ++i) {
        if (inSlab(face, d, i, mesh)) {
            outIndices.push_back(i);
        }
    }
}

bool OctaState::inSlab(OctaFace face, int depth, int slot, const OctaMesh& mesh) {
    const int d = std::clamp(depth, 1, kLatticeOrder);
    const Vec3 n = faceOutwardNormal(face);
    float maxDot = -1e10f;
    float minDot = 1e10f;
    for (int i = 0; i < kStickerCount; ++i) {
        const float pd = dot(mesh.centroids[static_cast<std::size_t>(i)], n);
        maxDot = std::max(maxDot, pd);
        minDot = std::min(minDot, pd);
    }
    const float span = std::max(maxDot - minDot, 1e-6f);
    const float threshold = maxDot - (static_cast<float>(d) / static_cast<float>(kLatticeOrder)) * span;
    const float pd = dot(mesh.centroids[static_cast<std::size_t>(slot)], n);
    return pd >= threshold - 1e-4f;
}

void OctaState::applyTurnPermutation(OctaFace face, int depth, int dir) {
    std::vector<int> slab;
    OctaMesh meshStub{};
    meshStub.centroids = centroids_;
    stickersInSlab(face, depth, meshStub, slab);

    const Vec3 axis = faceOutwardNormal(face);
    const float ang = static_cast<float>(dir) * (120.f * 3.14159265f / 180.f);

    std::array<int, kStickerCount> next = color_;
    for (int destSlot : slab) {
        const Vec3 p = centroids_[static_cast<std::size_t>(destSlot)];
        const Vec3 pSrc = rotateAroundAxis(p, axis, -ang);
        const int srcSlot = nearestStickerSlot(pSrc, centroids_);
        next[static_cast<std::size_t>(destSlot)] = color_[static_cast<std::size_t>(srcSlot)];
    }
    color_ = next;
}

void OctaState::apply(const OctaMove& m) {
    history_.push_back(m);
    applyTurnPermutation(m.face, static_cast<int>(m.depth), static_cast<int>(m.dir));
}

void OctaState::undo() {
    if (history_.empty()) {
        return;
    }
    const OctaMove last = history_.back();
    history_.pop_back();
    OctaMove inv = last;
    inv.dir = static_cast<std::int8_t>(-inv.dir);
    applyTurnPermutation(inv.face, static_cast<int>(inv.depth), static_cast<int>(inv.dir));
}

void OctaState::autoComplete() {
    reset();
}

void OctaState::scramble(int moves, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> faceDist(0, kOctaFaces - 1);
    std::uniform_int_distribution<int> depthDist(1, kLatticeOrder);
    std::uniform_int_distribution<int> dirDist(0, 2);

    for (int i = 0; i < moves; ++i) {
        OctaMove m{};
        m.face = static_cast<OctaFace>(faceDist(rng));
        m.depth = static_cast<std::uint8_t>(depthDist(rng));
        m.dir = static_cast<std::int8_t>(dirDist(rng) == 0 ? 1 : -1);
        applyTurnPermutation(m.face, static_cast<int>(m.depth), static_cast<int>(m.dir));
    }
    history_.clear();
}
