/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Noise.h"

#include <api/type/Random.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

namespace v3d::render::offline {

namespace {

/** The lattice's hash: a permutation of 0 to 255, written twice so an index never wraps. **/
std::array<int, 512> shuffled() {
    std::array<int, 512> table {};
    for (int i = 0; i < 256; i++) {
        table[static_cast<std::size_t>(i)] = i;
    }
    v3d::type::Random random(0x5EEDF00Du);
    for (uint32_t i = 255; i > 0; i--) {
        std::swap(table[i], table[random.below(i + 1)]);
    }
    for (std::size_t i = 0; i < 256; i++) {
        table[i + 256] = table[i];
    }
    return table;
}

const std::array<int, 512> & permutation() {
    static const std::array<int, 512> table = shuffled();
    return table;
}

float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

/**
 * The dot product of the offset with one of the twelve edge directions of a cube, picked by
 * the low four bits of the hash; the four that repeat keep the choice a mask rather than a
 * division.
 **/
float gradient(int hash, float x, float y, float z) {
    const int h = hash & 15;
    const float u = h < 8 ? x : y;
    float v = z;
    if (h < 4) {
        v = y;
    } else if (h == 12 || h == 14) {
        v = x;
    }
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

};  // namespace

float noise(const glm::vec3 & point) {
    // a point that is not finite has no cell, and reads the value every lattice point has
    const bool finite = std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
    if (!finite) {
        return 0.5f;
    }
    const std::array<int, 512> & p = permutation();
    const float fx = std::floor(point.x);
    const float fy = std::floor(point.y);
    const float fz = std::floor(point.z);
    // the lattice repeats every 256 cells, so a cell is reduced by that period before it
    // becomes an int, which a coordinate past the range of an int could not
    const int cx = static_cast<int>(std::fmod(fx, 256.0f)) & 255;
    const int cy = static_cast<int>(std::fmod(fy, 256.0f)) & 255;
    const int cz = static_cast<int>(std::fmod(fz, 256.0f)) & 255;
    const float x = point.x - fx;
    const float y = point.y - fy;
    const float z = point.z - fz;
    const float u = fade(x);
    const float v = fade(y);
    const float w = fade(z);

    const auto at = [&p](int index) { return p[static_cast<std::size_t>(index)]; };
    const int a = at(cx) + cy;
    const int aa = at(a) + cz;
    const int ab = at(a + 1) + cz;
    const int b = at(cx + 1) + cy;
    const int ba = at(b) + cz;
    const int bb = at(b + 1) + cz;

    const float value = lerp(w,
        lerp(v, lerp(u, gradient(at(aa), x, y, z), gradient(at(ba), x - 1.0f, y, z)),
            lerp(u, gradient(at(ab), x, y - 1.0f, z), gradient(at(bb), x - 1.0f, y - 1.0f, z))),
        lerp(v, lerp(u, gradient(at(aa + 1), x, y, z - 1.0f), gradient(at(ba + 1), x - 1.0f, y, z - 1.0f)),
            lerp(u, gradient(at(ab + 1), x, y - 1.0f, z - 1.0f),
                gradient(at(bb + 1), x - 1.0f, y - 1.0f, z - 1.0f))));
    // the improved noise's range is about [-1, 1]; its extremes reach a little past it, and
    // SL's noise never leaves [0, 1]
    return std::clamp(0.5f + 0.5f * value, 0.0f, 1.0f);
}

};  // namespace v3d::render::offline
