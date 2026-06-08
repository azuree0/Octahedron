// Vec3 + Rodrigues (mesh / slab twist).

#pragma once

#include <cmath>

struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Vec3() = default;
    Vec3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}

    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
};

inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(Vec3 v) {
    return std::sqrt(dot(v, v));
}

inline Vec3 normalize(Vec3 v) {
    const float len = length(v);
    if (len < 1e-20f) {
        return {0.f, 0.f, 0.f};
    }
    return v * (1.f / len);
}

inline Vec3 rotateAroundAxis(Vec3 v, Vec3 axisUnit, float angleRad) {
    const float c = std::cos(angleRad);
    const float s = std::sin(angleRad);
    const Vec3 k = axisUnit;
    const float t = 1.f - c;
    return {v.x * (t * k.x * k.x + c) + v.y * (t * k.x * k.y - s * k.z) +
                v.z * (t * k.x * k.z + s * k.y),
            v.x * (t * k.x * k.y + s * k.z) + v.y * (t * k.y * k.y + c) +
                v.z * (t * k.y * k.z - s * k.x),
            v.x * (t * k.x * k.z - s * k.y) + v.y * (t * k.y * k.z + s * k.x) +
                v.z * (t * k.z * k.z + c)};
}
