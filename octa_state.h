// Puzzle state: sticker permutations on 8-face octahedron lattice (indices align with octa_mesh).

#pragma once

#include "math3.h"

#include <array>
#include <cstdint>
#include <vector>

inline constexpr int kLatticeOrder = 6;
inline constexpr int kOctaFaces = 8;
inline constexpr int kTrisPerFace = kLatticeOrder * kLatticeOrder;
inline constexpr int kStickerCount = kOctaFaces * kTrisPerFace;

enum class OctaFace : std::uint8_t {
    Face1 = 0,
    Face2 = 1,
    Face3 = 2,
    Face4 = 3,
    Face5 = 4,
    Face6 = 5,
    Face7 = 6,
    Face8 = 7
};

inline int octaStateIdx(OctaFace face, int row, int col) {
    return static_cast<int>(face) * kTrisPerFace + row * kLatticeOrder + col;
}

struct OctaMove {
    OctaFace face = OctaFace::Face1;
    std::uint8_t depth = 1;
    std::int8_t dir = 1;
};

struct OctaMesh;

class OctaState {
public:
    explicit OctaState(const OctaMesh& mesh);

    void reset();
    void apply(const OctaMove& m);
    void undo();
    void autoComplete();
    void scramble(int moves, unsigned seed);

    const std::array<int, kStickerCount>& colors() const { return color_; }

    static void stickersInSlab(OctaFace face, int depth, const OctaMesh& mesh, std::vector<int>& outIndices);
    static bool inSlab(OctaFace face, int depth, int slot, const OctaMesh& mesh);

    static Vec3 faceOutwardNormal(OctaFace face);

private:
    void applyTurnPermutation(OctaFace face, int depth, int dir);

    std::array<int, kStickerCount> color_{};
    std::array<Vec3, kStickerCount> centroids_{};
    std::vector<OctaMove> history_;
};
