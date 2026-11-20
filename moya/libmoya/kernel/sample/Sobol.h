/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Hash.h>

#include <cstdint>

namespace v3d::moya::kernel::sample {

/** The bits of a sample's index that the generator matrices read. **/
inline constexpr std::uint32_t kSobolColumns = 64;

/** The place of a 32-bit value's highest bit, which holds a sample's first binary digit. **/
inline constexpr std::uint32_t kTopBit = 31;

/** The largest float below one, which a sample never exceeds. **/
inline constexpr float kOneMinusEpsilon = 0x1.FFFFFEp-1F;

/** The multipliers of owen()'s hash, in the order it applies them. **/
inline constexpr std::uint32_t kOwenFirst = 0x3D20ADEAU;
inline constexpr std::uint32_t kOwenSecond = 0x05526C56U;
inline constexpr std::uint32_t kOwenThird = 0x53A22864U;

/**
 * The generator matrices of the Sobol sequence's first two dimensions, one 32-bit column for each
 * bit of a sample's index. Bit 31 of a column is the sample's first binary digit.
 **/
struct SobolMatrices {
    std::uint32_t columns[2][kSobolColumns] {};

    /**
     * The first two dimensions' matrices, which are the same in every construction of the
     * sequence. The first is the van der Corput sequence's: index bit c gives output digit c.
     * The second is Pascal's triangle mod 2: index bit c gives output digit r when the binomial
     * coefficient C(c, r) is odd. By Lucas's theorem, C(c, r) is odd when r's bits are a subset
     * of c's. Index bits from 32 on reach only the second's leading digits.
     **/
    MOYA_KERNEL static constexpr SobolMatrices make() {
        SobolMatrices out;
        for (std::uint32_t column = 0; column < kSobolColumns; ++column) {
            if (column < 32U) {
                out.columns[0][column] = 1U << (kTopBit - column);
            }
            for (std::uint32_t row = 0; row < 32U; ++row) {
                if ((row & column) == row) {
                    out.columns[1][column] |= 1U << (kTopBit - row);
                }
            }
        }
        return out;
    }
};

/** The generator matrices sobol() reads. **/
MOYA_KERNEL_DATA inline constexpr SobolMatrices kSobol = SobolMatrices::make();

/** The Sobol sequence's value at an index, in dimension 0 or 1, as 32 binary digits. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, then the dimension
MOYA_KERNEL inline std::uint32_t sobol(std::uint64_t index, int dimension) {
    std::uint32_t out = 0;
    for (std::uint32_t column = 0; index != 0; index >>= 1U, ++column) {
        if ((index & 1U) != 0) {
            out ^= kSobol.columns[dimension][column];
        }
    }
    return out;
}

/**
 * An Owen scrambling of 32 binary digits by a seed. Each digit is flipped or kept by a hash of the
 * digits before it, so a set of samples stratified in binary intervals stays stratified, and
 * samples scrambled by different seeds are independent. The hash is Vegdahl's improvement of
 * Laine and Karras's, which works on the reversed digits.
 **/
MOYA_KERNEL inline std::uint32_t owen(std::uint32_t value, std::uint32_t seed) {
    value = reverseBits(value);
    value ^= value * kOwenFirst;
    value += seed;
    value *= (seed >> 16U) | 1U;
    value ^= value * kOwenSecond;
    value ^= value * kOwenThird;
    return reverseBits(value);
}

/** 32 binary digits as a float in [0, 1). **/
MOYA_KERNEL inline float toUnit(std::uint32_t value) {
    const float unit = static_cast<float>(value) * 0x1p-32F;
    return unit < kOneMinusEpsilon ? unit : kOneMinusEpsilon;
}

};  // namespace v3d::moya::kernel::sample
