/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::sample {

/** The shifts and multipliers of SplitMix64's finaliser, in the order mix() applies them. **/
inline constexpr std::uint32_t kMixShiftFirst = 30;
inline constexpr std::uint64_t kMixMultiplierFirst = 0xBF58476D1CE4E5B9ULL;
inline constexpr std::uint32_t kMixShiftSecond = 27;
inline constexpr std::uint64_t kMixMultiplierSecond = 0x94D049BB133111EBULL;
inline constexpr std::uint32_t kMixShiftThird = 31;

/**
 * The masks reverseBits() swaps by, for groups of 1, 2, 4, 8 and 16 bits. Each keeps the lower
 * group of every pair.
 **/
MOYA_KERNEL_DATA inline constexpr std::uint32_t kReverseMasks[5] = {
    0x55555555U, 0x33333333U, 0x0F0F0F0FU, 0x00FF00FFU, 0x0000FFFFU,
};

/** The masks spreadBits() keeps after each shift, for shifts of 16, 8, 4, 2 and 1 bits. **/
MOYA_KERNEL_DATA inline constexpr std::uint64_t kSpreadMasks[5] = {
    0x0000FFFF0000FFFFULL, 0x00FF00FF00FF00FFULL, 0x0F0F0F0F0F0F0F0FULL, 0x3333333333333333ULL, 0x5555555555555555ULL,
};

/**
 * Mixes the bits of a 64-bit value, so that every input bit affects every output bit. It is
 * SplitMix64's finaliser, a bijection, so distinct inputs give distinct outputs.
 **/
MOYA_KERNEL inline std::uint64_t mix(std::uint64_t value) {
    value ^= value >> kMixShiftFirst;
    value *= kMixMultiplierFirst;
    value ^= value >> kMixShiftSecond;
    value *= kMixMultiplierSecond;
    value ^= value >> kMixShiftThird;
    return value;
}

/** A 32-bit value with its bits in reverse order: bit 0 becomes bit 31. **/
MOYA_KERNEL inline std::uint32_t reverseBits(std::uint32_t value) {
    std::uint32_t width = 1;
    for (const std::uint32_t mask : kReverseMasks) {
        value = ((value >> width) & mask) | ((value & mask) << width);
        width <<= 1U;
    }
    return value;
}

/** A 32-bit value's bits moved to the even bits of a 64-bit one: bit i becomes bit 2i. **/
MOYA_KERNEL inline std::uint64_t spreadBits(std::uint32_t bits) {
    std::uint64_t value = bits;
    std::uint32_t width = 16;
    for (const std::uint64_t mask : kSpreadMasks) {
        value = (value | (value << width)) & mask;
        width >>= 1U;
    }
    return value;
}

/** A pixel's index on the Morton curve: the bits of x and y interleaved, with x's bit 0 lowest. **/
MOYA_KERNEL inline std::uint64_t morton(std::uint32_t x, std::uint32_t y) {
    return spreadBits(x) | (spreadBits(y) << 1U);
}

};  // namespace v3d::moya::kernel::sample
