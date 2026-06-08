// Geometry: regular octahedron with order-6 barycentric lattice per triangular face.

#include "octa_mesh.h"

#include <cassert>
#include <vector>

namespace {

int latticeLinearIndex(int n, int a, int b) {
    int id = 0;
    for (int aa = 0; aa < a; ++aa) {
        id += (n - aa + 1);
    }
    return id + b;
}

void appendLatticeTriangles(int n, std::vector<std::array<int, 3>>& out) {
    auto idx = [n](int a, int b) { return latticeLinearIndex(n, a, b); };
    for (int a = 0; a <= n; ++a) {
        for (int b = 0; b <= n - a; ++b) {
            const int c = n - a - b;
            if (c >= 1) {
                out.push_back({idx(a, b), idx(a, b + 1), idx(a + 1, b)});
            }
            if (c >= 2) {
                out.push_back({idx(a, b + 1), idx(a + 1, b), idx(a + 1, b + 1)});
            }
        }
    }
}

void assignFaceCorners(std::array<std::array<int, 3>, kOctaFaces>& out) {
    // Vertices: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z. CCW outward per face.
    // Each face uses three mutually adjacent axis tips (never +Z with -Z, or +Y with -Y).
    out[0] = {0, 2, 4}; // (+X, +Y, +Z)
    out[1] = {0, 5, 2}; // (+X, +Y, -Z)
    out[2] = {0, 3, 5}; // (+X, -Y, -Z)
    out[3] = {0, 4, 3}; // (+X, -Y, +Z)
    out[4] = {1, 4, 2}; // (-X, +Y, +Z)
    out[5] = {1, 2, 5}; // (-X, +Y, -Z)
    out[6] = {1, 5, 3}; // (-X, -Y, -Z)
    out[7] = {1, 3, 4}; // (-X, -Y, +Z)
}

std::array<Vec3, 6> rawCorners() {
    return {Vec3{1.f, 0.f, 0.f},  Vec3{-1.f, 0.f, 0.f}, Vec3{0.f, 1.f, 0.f},
            Vec3{0.f, -1.f, 0.f}, Vec3{0.f, 0.f, 1.f},  Vec3{0.f, 0.f, -1.f}};
}

void latticePoints(int n, Vec3 p0, Vec3 p1, Vec3 p2, std::vector<Vec3>& outPts) {
    outPts.clear();
    for (int a = 0; a <= n; ++a) {
        for (int b = 0; b <= n - a; ++b) {
            const int c = n - a - b;
            const Vec3 p = (p0 * static_cast<float>(a) + p1 * static_cast<float>(b) + p2 * static_cast<float>(c)) *
                           (1.0f / static_cast<float>(n));
            outPts.push_back(p);
        }
    }
}

Vec3 triangleNormalOutward(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 faceOut) {
    Vec3 n = cross(p1 - p0, p2 - p0);
    if (dot(n, faceOut) < 0.f) {
        std::swap(p1, p2);
        n = cross(p1 - p0, p2 - p0);
    }
    return normalize(n);
}

void buildStickersForFace(int f, OctaMesh& mesh, const std::vector<std::array<int, 3>>& triIdx,
                          std::vector<Vec3>& lattice) {
    const int i0 = mesh.faceCorners[static_cast<std::size_t>(f)][0];
    const int i1 = mesh.faceCorners[static_cast<std::size_t>(f)][1];
    const int i2 = mesh.faceCorners[static_cast<std::size_t>(f)][2];
    const Vec3 p0 = mesh.corners[static_cast<std::size_t>(i0)];
    const Vec3 p1 = mesh.corners[static_cast<std::size_t>(i1)];
    const Vec3 p2 = mesh.corners[static_cast<std::size_t>(i2)];
    const Vec3 faceCenter = (p0 + p1 + p2) * (1.f / 3.f);
    const Vec3 faceOut = normalize(faceCenter);

    latticePoints(kLatticeOrder, p0, p1, p2, lattice);

    for (int t = 0; t < kTrisPerFace; ++t) {
        const int g = f * kTrisPerFace + t;
        const int ia = triIdx[static_cast<std::size_t>(t)][0];
        const int ib = triIdx[static_cast<std::size_t>(t)][1];
        const int ic = triIdx[static_cast<std::size_t>(t)][2];
        Vec3 va = lattice[static_cast<std::size_t>(ia)];
        Vec3 vb = lattice[static_cast<std::size_t>(ib)];
        Vec3 vc = lattice[static_cast<std::size_t>(ic)];
        Vec3 nrm = triangleNormalOutward(va, vb, vc, faceOut);
        if (dot(nrm, faceOut) <= 0.f) {
            std::swap(vb, vc);
            nrm = triangleNormalOutward(va, vb, vc, faceOut);
        }
        mesh.triangles[static_cast<std::size_t>(g)].pos = {va, vb, vc};
        mesh.triangles[static_cast<std::size_t>(g)].normal = nrm;
        mesh.centroids[static_cast<std::size_t>(g)] = (va + vb + vc) * (1.f / 3.f);
    }
}

} // namespace

OctaMesh buildOctaMesh(float halfExtent) {
    OctaMesh mesh{};
    assignFaceCorners(mesh.faceCorners);

    std::array<Vec3, 6> raw = rawCorners();
    for (int i = 0; i < 6; ++i) {
        mesh.corners[static_cast<std::size_t>(i)] = normalize(raw[static_cast<std::size_t>(i)]) * halfExtent;
    }

    const int n = kLatticeOrder;
    std::vector<std::array<int, 3>> triIdx;
    triIdx.reserve(static_cast<std::size_t>(n * n));
    appendLatticeTriangles(n, triIdx);
    assert(triIdx.size() == static_cast<std::size_t>(n * n));

    std::vector<Vec3> lattice;
    for (int f = 0; f < kOctaFaces; ++f) {
        buildStickersForFace(f, mesh, triIdx, lattice);
    }

    return mesh;
}

Vec3 stickerSlotCenterInMesh(float halfExtent, int slot) {
    const OctaMesh mesh = buildOctaMesh(halfExtent);
    return mesh.centroids[static_cast<std::size_t>(slot)];
}
