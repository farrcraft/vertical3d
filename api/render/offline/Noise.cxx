/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Noise.h"

#include <api/type/Random.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace v3d::render::offline {

namespace {

/** The seed of the lattice's shuffle. Every render sees the same noise. **/
constexpr uint32_t PERMUTATION_SEED = 0x5EEDF00DU;

/** The lattice's hash: a permutation of 0 to 255, written twice so an index never wraps. **/
std::array<int, 512> shuffled() {
    std::array<int, 512> table {};
    for (int i = 0; i < 256; i++) {
        table[static_cast<std::size_t>(i)] = i;
    }
    v3d::type::Random random(PERMUTATION_SEED);
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
    // the coefficients of 6t^5 - 15t^4 + 10t^3, whose first and second derivatives are zero
    // at 0 and 1
    constexpr float quintic = 6.0F;
    constexpr float quartic = 15.0F;
    constexpr float cubic = 10.0F;
    return t * t * t * ((t * ((t * quintic) - quartic)) + cubic);
}

float lerp(float t, float from, float to) {
    return from + (t * (to - from));
}

/**
 * The dot product of the offset with one of the twelve edge directions of a cube, picked by
 * the low four bits of the hash. Four of the directions repeat, so the choice is a mask
 * rather than a division.
 **/
float gradient(int hash, const glm::vec3 & offset) {
    // hashes 12 to 15 repeat four of the directions. 12 (x + y) and 14 (y - x) lie in the xy
    // plane, so their second component is x. 13 and 15 keep z
    constexpr int repeatXPlusY = 12;
    constexpr int repeatYMinusX = 14;
    const int bits = hash & 15;
    const float u = bits < 8 ? offset.x : offset.y;
    float v = offset.z;
    if (bits < 4) {
        v = offset.y;
    } else if (bits == repeatXPlusY || bits == repeatYMinusX) {
        v = offset.x;
    }
    return ((bits & 1) == 0 ? u : -u) + ((bits & 2) == 0 ? v : -v);
}

};  // namespace

float noise(const glm::vec3 & point) {
    // a point that is not finite has no cell, and reads the value every lattice point has
    const bool finite = std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
    if (!finite) {
        return 0.5F;
    }
    const std::array<int, 512> & table = permutation();
    const float fx = std::floor(point.x);
    const float fy = std::floor(point.y);
    const float fz = std::floor(point.z);
    // the lattice repeats every 256 cells, so a cell is reduced by that period before it
    // becomes an int, which a coordinate past the range of an int could not
    const int cx = static_cast<int>(std::fmod(fx, 256.0F)) & 255;  // checked: fx is finite, so the remainder lies in (-256, 256)
    const int cy = static_cast<int>(std::fmod(fy, 256.0F)) & 255;  // checked: fy is finite, so the remainder lies in (-256, 256)
    const int cz = static_cast<int>(std::fmod(fz, 256.0F)) & 255;  // checked: fz is finite, so the remainder lies in (-256, 256)
    const float x = point.x - fx;
    const float y = point.y - fy;
    const float z = point.z - fz;
    const float u = fade(x);
    const float v = fade(y);
    const float w = fade(z);

    const auto at = [&table](int index) { return table[static_cast<std::size_t>(index)]; };
    const int hashA = at(cx) + cy;
    const int aa = at(hashA) + cz;
    const int ab = at(hashA + 1) + cz;
    const int hashB = at(cx + 1) + cy;
    const int ba = at(hashB) + cz;
    const int bb = at(hashB + 1) + cz;

    const float value = lerp(w,
        lerp(v, lerp(u, gradient(at(aa), {x, y, z}), gradient(at(ba), {x - 1.0F, y, z})),
            lerp(u, gradient(at(ab), {x, y - 1.0F, z}), gradient(at(bb), {x - 1.0F, y - 1.0F, z}))),
        lerp(v, lerp(u, gradient(at(aa + 1), {x, y, z - 1.0F}), gradient(at(ba + 1), {x - 1.0F, y, z - 1.0F})),
            lerp(u, gradient(at(ab + 1), {x, y - 1.0F, z - 1.0F}),
                gradient(at(bb + 1), {x - 1.0F, y - 1.0F, z - 1.0F}))));
    // the improved noise's range is about [-1, 1]; its extremes reach a little past it, and
    // SL's noise never leaves [0, 1]
    return std::clamp(0.5F + (0.5F * value), 0.0F, 1.0F);
}

};  // namespace v3d::render::offline
