/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::program {

/** A point of a noise, of two components or of three, and the whole coordinates of its cell. **/
template <glm::length_t N>
using NoisePoint = glm::vec<N, float, glm::defaultp>;

template <glm::length_t N>
using NoiseCell = glm::vec<N, std::int32_t, glm::defaultp>;

/** What a hash's seed starts from, before four times its words and 13 are added. **/
inline constexpr std::uint32_t kHashSeed = 0xdeadbeefU;
inline constexpr std::uint32_t kHashInitial = 13U;

/** What Perlin noise is scaled by to fill about [-1, 1], in two dimensions and in three. **/
inline constexpr float kPerlinScale2 = 0.6616F;
inline constexpr float kPerlinScale3 = 0.9820F;

/** Where the fourth channel of a noise is read, from the point, in two dimensions and in three. **/
inline constexpr glm::vec2 kFourthNoise2 { 19.0F, 73.0F };
inline constexpr glm::vec3 kFourthNoise3 { 19.0F, 73.0F, 29.0F };

/** Where a fractal's channels after its first are read, from the point. **/
inline constexpr glm::vec2 kLaterFractal2 { 19.0F, 193.0F };
inline constexpr glm::vec3 kLaterFractal3 { 19.0F, 193.0F, 17.0F };

/** The most octaves a fractal sums. **/
inline constexpr std::int32_t kMostOctaves = 32;

/** The largest hash, which a hash is divided by to give a number from zero to one. **/
inline constexpr std::uint32_t kLargestHash = 0xffffffffU;

/** The coefficients of fade's quintic: of t to the fifth, the fourth and the third. **/
inline constexpr float kFadeFifth = 6.0F;
inline constexpr float kFadeFourth = 15.0F;
inline constexpr float kFadeThird = 10.0F;

/** The two hashes of eight or more whose gradient's second part is along x. **/
inline constexpr std::uint32_t kSecondAlongX = 12U;
inline constexpr std::uint32_t kAlsoSecondAlongX = 14U;

/** The squared distance a Worley noise starts its nearest from. **/
inline constexpr float kFarthest = 1.0e6F;

/** The least and the most a cell's whole coordinate may be: what a 32-bit integer holds. **/
inline constexpr float kLeastWhole = -2147483648.0F;
inline constexpr float kMostWhole = 2147483520.0F;

/** The steps of lookup3's mix and of its final mix, and the bits each step turns a word by. **/
inline constexpr std::uint32_t kMixSteps = 6;
inline constexpr std::uint32_t kFinalSteps = 7;
MOYA_KERNEL_DATA inline constexpr std::uint32_t kMixTurns[kMixSteps] = { 4U, 6U, 8U, 16U, 19U, 4U };
MOYA_KERNEL_DATA inline constexpr std::uint32_t kFinalTurns[kFinalSteps] = { 14U, 11U, 25U, 16U, 4U, 14U, 24U };

/** A word turned left by a number of bits. **/
MOYA_KERNEL inline std::uint32_t rotated(std::uint32_t word, std::uint32_t bits) {
    return (word << bits) | (word >> (32U - bits));
}

/**
 * lookup3's mix of three words. Step k changes word k mod 3, by the word before it, and adds the
 * word after it to the word before.
 **/
MOYA_KERNEL inline void jenkinsMix(std::uint32_t * words) {
    for (std::uint32_t step = 0; step < kMixSteps; ++step) {
        const std::uint32_t changed = step % 3U;
        const std::uint32_t before = (changed + 2U) % 3U;
        const std::uint32_t after = (changed + 1U) % 3U;
        words[changed] -= words[before];
        words[changed] ^= rotated(words[before], kMixTurns[step]);
        words[before] += words[after];
    }
}

/**
 * lookup3's final mix of three words, whose last is the hash. Step k changes word (k + 2) mod 3,
 * by the word before it.
 **/
MOYA_KERNEL inline std::uint32_t jenkinsFinal(std::uint32_t * words) {
    for (std::uint32_t step = 0; step < kFinalSteps; ++step) {
        const std::uint32_t changed = (step + 2U) % 3U;
        const std::uint32_t before = (changed + 2U) % 3U;
        words[changed] ^= words[before];
        words[changed] -= rotated(words[before], kFinalTurns[step]);
    }
    return words[2];
}

/**
 * lookup3's hash of one to four words. Up to three are added to its three words in order. A
 * fourth is added to the first after the three are mixed.
 **/
MOYA_KERNEL inline std::uint32_t hashWords(const std::uint32_t * given, std::uint32_t count) {
    const std::uint32_t seed = kHashSeed + (count << 2U) + kHashInitial;
    std::uint32_t words[3] = { seed, seed, seed };
    for (std::uint32_t i = 0; i < count && i < 3U; ++i) {
        words[i] += given[i];
    }
    if (count > 3U) {
        jenkinsMix(words);
        words[0] += given[3];
    }
    return jenkinsFinal(words);
}

/** The hash of a cell's whole coordinates, and of them with one more word after. **/
template <glm::length_t N>
MOYA_KERNEL inline std::uint32_t hashOf(const NoiseCell<N> & cell) {
    std::uint32_t words[3] {};
    for (glm::length_t i = 0; i < N; ++i) {
        words[i] = static_cast<std::uint32_t>(cell[i]);
    }
    return hashWords(words, static_cast<std::uint32_t>(N));
}

template <glm::length_t N>
MOYA_KERNEL inline std::uint32_t hashOf(const NoiseCell<N> & cell, std::uint32_t last) {
    std::uint32_t words[4] {};
    for (glm::length_t i = 0; i < N; ++i) {
        words[i] = static_cast<std::uint32_t>(cell[i]);
    }
    words[N] = last;
    return hashWords(words, static_cast<std::uint32_t>(N) + 1U);
}

/** A hash as a number from zero to one, divided as 32-bit floats. **/
MOYA_KERNEL inline float unitOf(std::uint32_t hash) {
    return static_cast<float>(hash) / static_cast<float>(kLargestHash);
}

/** The whole number at or below a value, held to what a 32-bit integer holds. **/
MOYA_KERNEL inline std::int32_t wholeOf(float value) {
    const float below = std::fmin(std::fmax(std::floor(value), kLeastWhole), kMostWhole);
    return static_cast<std::int32_t>(below);  // checked: held to a 32-bit integer's range above
}

/** The cell a point lies in, and the point's offset from the cell's corner. **/
template <glm::length_t N>
MOYA_KERNEL inline NoiseCell<N> cellOf(const NoisePoint<N> & point, NoisePoint<N> & within) {
    NoiseCell<N> out { 0 };
    for (glm::length_t i = 0; i < N; ++i) {
        out[i] = wholeOf(point[i]);
        within[i] = point[i] - std::floor(point[i]);
    }
    return out;
}

/** Zero at zero and one at one, with its first and second derivatives zero at both. **/
MOYA_KERNEL inline float fade(float t) {
    return t * t * t * ((t * ((t * kFadeFifth) - kFadeFourth)) + kFadeThird);
}

/** A part of a gradient's dot product, negated where a bit of the hash is set. **/
MOYA_KERNEL inline float signedBy(float part, std::uint32_t hash, std::uint32_t bit) {
    return (hash & bit) != 0U ? -part : part;
}

/** An offset's dot product with a gradient of (+-1, +-2) or (+-2, +-1), by a hash's low bits. **/
MOYA_KERNEL inline float slope(std::uint32_t hash, const glm::vec2 & offset) {
    const std::uint32_t low = hash & 7U;
    const bool first = low < 4U;
    return signedBy(first ? offset.x : offset.y, low, 1U) + signedBy(2.0F * (first ? offset.y : offset.x), low, 2U);
}

/** The dot product of an offset with an edge of a cube, by a hash's low bits. **/
MOYA_KERNEL inline float slope(std::uint32_t hash, const glm::vec3 & offset) {
    const std::uint32_t low = hash & 15U;
    const float u = low < 8U ? offset.x : offset.y;
    float v = offset.z;
    if (low < 4U) {
        v = offset.y;
    } else if (low == kSecondAlongX || low == kAlsoSecondAlongX) {
        v = offset.x;
    }
    return signedBy(u, low, 1U) + signedBy(v, low, 2U);
}

/**
 * Perlin noise of a point, of one to three channels, from about -1 to 1. Each channel takes its
 * gradients from a byte of each corner's hash, so the first channel is the noise of one.
 **/
template <glm::length_t N>
MOYA_KERNEL inline glm::vec3 perlin(const NoisePoint<N> & point, std::uint32_t channels) {
    NoisePoint<N> within { 0.0F };
    const NoiseCell<N> cell = cellOf<N>(point, within);
    constexpr std::uint32_t kCorners = 1U << static_cast<std::uint32_t>(N);
    // a corner's bit i is its step along axis i
    glm::vec3 corners[kCorners] {};
    for (std::uint32_t corner = 0; corner < kCorners; ++corner) {
        NoiseCell<N> at = cell;
        NoisePoint<N> offset = within;
        for (glm::length_t i = 0; i < N; ++i) {
            const std::uint32_t step = (corner >> static_cast<std::uint32_t>(i)) & 1U;
            at[i] += static_cast<std::int32_t>(step);
            offset[i] -= static_cast<float>(step);
        }
        const std::uint32_t hash = hashOf<N>(at);
        for (std::uint32_t channel = 0; channel < channels && channel < 3U; ++channel) {
            corners[corner][static_cast<glm::length_t>(channel)] = slope((hash >> (8U * channel)) & 0xffU, offset);
        }
    }
    // mix along x, then y, then z, each pair of corners differing in the lowest bit left
    std::uint32_t left = kCorners;
    for (glm::length_t i = 0; i < N; ++i) {
        const float t = fade(within[i]);
        left /= 2U;
        for (std::uint32_t half = 0; half < left; ++half) {
            corners[half] = (corners[2U * half] * (1.0F - t)) + (corners[(2U * half) + 1U] * t);
        }
    }
    return corners[0] * (N == 2 ? kPerlinScale2 : kPerlinScale3);
}

/**
 * Octaves of Perlin noise summed: the first at the point and of weight one, and each after it at
 * lacunarity times the point before and diminish times the weight before.
 **/
template <glm::length_t N>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the octaves'
MOYA_KERNEL inline glm::vec3 fractal(NoisePoint<N> point, std::uint32_t channels, std::int32_t octaves, float lacunarity, float diminish) {
    glm::vec3 sum { 0.0F };
    float weight = 1.0F;
    for (std::int32_t octave = 0; octave < octaves && octave < kMostOctaves; ++octave) {
        sum += weight * perlin<N>(point, channels);
        weight *= diminish;
        point *= lacunarity;
    }
    return sum;
}

/** Cell noise: a number from zero to one for the unit cell a point lies in. **/
template <glm::length_t N>
MOYA_KERNEL inline float cell(const NoisePoint<N> & point) {
    NoisePoint<N> within { 0.0F };
    return unitOf(hashOf<N>(cellOf<N>(point, within)));
}

/** Cell noise of three channels: the hashes of the cell's coordinates and zero, one or two. **/
template <glm::length_t N>
MOYA_KERNEL inline glm::vec3 cellChannels(const NoiseCell<N> & at) {
    return { unitOf(hashOf<N>(at, 0U)), unitOf(hashOf<N>(at, 1U)), unitOf(hashOf<N>(at, 2U)) };
}

template <glm::length_t N>
MOYA_KERNEL inline glm::vec3 cellChannels(const NoisePoint<N> & point) {
    NoisePoint<N> within { 0.0F };
    return cellChannels<N>(cellOf<N>(point, within));
}

/** Keeps the three least squared distances in order as another is found. **/
MOYA_KERNEL inline void keepLeast(glm::vec3 & least, float distance) {
    if (distance < least.x) {
        least = { distance, least.x, least.y };
    } else if (distance < least.y) {
        least = { least.x, distance, least.y };
    } else if (distance < least.z) {
        least.z = distance;
    }
}

/**
 * Worley noise of one to three channels: the distances to the nearest feature points of the cells
 * about a point's, nearest first, or in the solid style the cell noise at the nearest.
 **/
template <glm::length_t N>
MOYA_KERNEL inline glm::vec3 worley(const NoisePoint<N> & point, float jitter, bool solid, std::uint32_t channels) {
    NoisePoint<N> within { 0.0F };
    const NoiseCell<N> home = cellOf<N>(point, within);
    std::uint32_t neighbours = 1U;
    for (glm::length_t i = 0; i < N; ++i) {
        neighbours *= 3U;
    }
    glm::vec3 least { kFarthest };
    NoisePoint<N> nearest { 0.0F };
    // the x offset outermost, and the last axis's innermost
    for (std::uint32_t neighbour = 0; neighbour < neighbours; ++neighbour) {
        NoiseCell<N> step { 0 };
        std::uint32_t digits = neighbour;
        for (glm::length_t i = N - 1; i >= 0; --i) {
            step[i] = static_cast<std::int32_t>(digits % 3U) - 1;
            digits /= 3U;
        }
        const glm::vec3 noise = cellChannels<N>(home + step);
        NoisePoint<N> toFeature { 0.0F };
        for (glm::length_t i = 0; i < N; ++i) {
            toFeature[i] = static_cast<float>(step[i]) + 0.5F + (jitter * (noise[i] - 0.5F)) - within[i];
        }
        const float distance = glm::dot(toFeature, toFeature);
        if (distance < least.x) {
            nearest = toFeature;
        }
        keepLeast(least, distance);
    }
    if (solid) {
        return channels == 1U ? glm::vec3(cell<N>(point + nearest)) : cellChannels<N>(point + nearest);
    }
    return glm::sqrt(least);
}

};  // namespace v3d::moya::kernel::program
