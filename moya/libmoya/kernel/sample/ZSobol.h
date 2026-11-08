/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Hash.h>
#include <moya/libmoya/kernel/sample/Sobol.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::sample {

/**
 * The 24 orders of four base-4 digits, one of which reorders each digit of a sample's index.
 **/
MOYA_KERNEL_DATA inline constexpr std::uint8_t kDigitOrders[24][4] = {
    { 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 2, 1, 3 }, { 0, 2, 3, 1 }, { 0, 3, 2, 1 }, { 0, 3, 1, 2 },
    { 1, 0, 2, 3 }, { 1, 0, 3, 2 }, { 1, 2, 0, 3 }, { 1, 2, 3, 0 }, { 1, 3, 2, 0 }, { 1, 3, 0, 2 },
    { 2, 1, 0, 3 }, { 2, 1, 3, 0 }, { 2, 0, 1, 3 }, { 2, 0, 3, 1 }, { 2, 3, 0, 1 }, { 2, 3, 1, 0 },
    { 3, 1, 2, 0 }, { 3, 1, 0, 2 }, { 3, 2, 1, 0 }, { 3, 2, 0, 1 }, { 3, 0, 2, 1 }, { 3, 0, 1, 2 },
};

/** The multiplier that spreads a dimension's number over the bits of the scrambling's hash. **/
inline constexpr std::uint64_t kDimensionSpread = 0x9E3779B97F4A7C15ULL;

/** The salt a dimension's number gives the hash that reorders a digit, once multiplied by it. **/
inline constexpr std::uint64_t kDimensionSalt = 0x55555555ULL;

/**
 * Ahmed and Wonka's ZSobol sampler. The pixels of an image are laid along the Morton curve, and
 * each pixel takes the next run of samples of one two-dimensional Sobol sequence. A pixel's
 * samples are then stratified among themselves, and neighbouring pixels draw from disjoint parts
 * of the sequence, so their errors differ.
 *
 * Every dimension draws from the sequence's first two dimensions, and two things tell them
 * apart. Each base-4 digit of the index is reordered by a hash of the dimension and the digits
 * above it. Each value is Owen scrambled by a hash of the dimension, the seed and the frame.
 *
 * A sample is a function of its pixel, its index within the pixel, its dimension, the seed and
 * the frame. The sampler holds no other state, so any thread draws any sample in any order and
 * gets the same value. Each draw, of one value or of two, takes a dimension of its own.
 **/
struct ZSobol {
    /** The base-2 logarithm of the samples each pixel takes. **/
    std::uint32_t log2Samples { 0 };

    /** The base-4 digits of a sample's index: enough for every pixel's samples. **/
    std::uint32_t digits { 0 };

    /** The seed and the frame, mixed. **/
    std::uint64_t scramble { 0 };

    ZSobol() = default;

    /**
     * A sampler for an image of the given resolution, with the given samples per pixel. A count
     * that is not a power of two is rounded up to one. Every frame takes its own samples.
     **/
    MOYA_KERNEL ZSobol(std::uint32_t samplesPerPixel, glm::uvec2 resolution, std::uint32_t seed, std::uint32_t frame) :
        log2Samples(log2Ceiling(samplesPerPixel)),
        digits(log2Ceiling(resolution.x > resolution.y ? resolution.x : resolution.y) + ((log2Samples + 1U) / 2U)),
        scramble(mix((static_cast<std::uint64_t>(seed) << 32U) | frame)) {
    }

    /** The samples each pixel takes. **/
    MOYA_KERNEL std::uint32_t samplesPerPixel() const {
        return 1U << log2Samples;
    }

    /** One value in [0, 1). **/
    MOYA_KERNEL float get1D(glm::uvec2 pixel, std::uint32_t sample, std::uint32_t dimension) const {
        const std::uint64_t at = index(pixel, sample, dimension);
        return toUnit(owen(sobol(at, 0), static_cast<std::uint32_t>(hash(dimension))));
    }

    /** Two values in [0, 1), stratified together. **/
    MOYA_KERNEL glm::vec2 get2D(glm::uvec2 pixel, std::uint32_t sample, std::uint32_t dimension) const {
        const std::uint64_t at = index(pixel, sample, dimension);
        const std::uint64_t seeds = hash(dimension);
        return {
            toUnit(owen(sobol(at, 0), static_cast<std::uint32_t>(seeds))),
            toUnit(owen(sobol(at, 1), static_cast<std::uint32_t>(seeds >> 32U))),
        };
    }

    /** The smallest n with 2^n at least the value. **/
    MOYA_KERNEL static std::uint32_t log2Ceiling(std::uint32_t value) {
        std::uint32_t log2 = 0;
        while ((1ULL << log2) < value) {
            ++log2;
        }
        return log2;
    }

    /** The Owen scrambling's seeds for a dimension: two 32-bit seeds, one for each value. **/
    MOYA_KERNEL std::uint64_t hash(std::uint32_t dimension) const {
        return mix(scramble ^ (kDimensionSpread * (static_cast<std::uint64_t>(dimension) + 1U)));
    }

    /**
     * The index into the Sobol sequence of a pixel's sample in a dimension. The pixel's Morton
     * index, followed by the sample's index, is read as base-4 digits from the highest. Each
     * digit is reordered by one of kDigitOrders, chosen by a hash of the digits above it and the
     * dimension. The digits above a pixel's own samples are its Morton index, so the pixel still
     * takes an aligned run of the sequence. When the samples per pixel are an odd power of two,
     * the lowest digit is a single bit, flipped or kept by the same kind of hash.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the sample, then the dimension
    MOYA_KERNEL std::uint64_t index(glm::uvec2 pixel, std::uint32_t sample, std::uint32_t dimension) const {
        const std::uint64_t ordered = (morton(pixel.x, pixel.y) << log2Samples) | sample;
        const std::uint64_t salt = kDimensionSalt * dimension;
        const std::uint32_t odd = log2Samples & 1U;
        std::uint64_t out = 0;
        for (std::uint32_t digit = digits; digit > odd; --digit) {
            const std::uint32_t shift = (2U * (digit - 1U)) - odd;
            const std::uint64_t value = (ordered >> shift) & 3U;
            const std::uint64_t above = ordered >> (shift + 2U);
            const std::uint64_t order = (mix(above ^ salt) >> 24U) % 24U;
            out |= static_cast<std::uint64_t>(kDigitOrders[order][value]) << shift;
        }
        if (odd != 0) {
            out |= (ordered & 1U) ^ (mix((ordered >> 1U) ^ salt) & 1U);
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::sample
