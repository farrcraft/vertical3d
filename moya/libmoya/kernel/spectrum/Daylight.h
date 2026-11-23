/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/CieDaylight.h>

#include <cmath>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::spectrum {

/** The correlated colour temperature, in kelvin, at which the daylight locus changes formula. **/
inline constexpr float kDaylightSplit = 7000.0F;

/**
 * The daylight locus's x at temperatures up to kDaylightSplit, as a cubic in 1 / T: the
 * coefficients of 1 / T^3, 1 / T^2 and 1 / T, then the constant.
 **/
MOYA_KERNEL_DATA inline constexpr float kDaylightCool[4] = { -4.6070e9F, 2.9678e6F, 0.09911e3F, 0.244063F };

/** The daylight locus's x above kDaylightSplit, in the same form. **/
MOYA_KERNEL_DATA inline constexpr float kDaylightWarm[4] = { -2.0064e9F, 1.9018e6F, 0.24748e3F, 0.23704F };

/** The daylight locus's y from its x: the coefficients of x^2 and x, then the constant. **/
MOYA_KERNEL_DATA inline constexpr float kDaylightY[3] = { -3.000F, 2.870F, -0.275F };

/**
 * The weights of S1 and S2 from a chromaticity, as CIE 15 gives them: for each of the
 * denominator, M1's numerator and M2's numerator, the constant, then the coefficients of x and y.
 **/
MOYA_KERNEL_DATA inline constexpr float kDaylightWeights[3][3] = {
    { 0.0241F, 0.2562F, -0.7341F },
    { -1.3515F, -1.7703F, 5.9114F },
    { 0.0300F, -31.4424F, 30.0717F },
};

/** CIE 15 rounds each weight to this many parts, three decimal places. **/
inline constexpr float kDaylightRounding = 1000.0F;

/**
 * The chromaticity of CIE daylight at a correlated colour temperature in kelvin, on the daylight
 * locus, which CIE 15 defines from 4000 K to 25000 K.
 **/
MOYA_KERNEL inline glm::vec2 daylightChromaticity(float temperature) {
    const float * cubic = temperature <= kDaylightSplit ? kDaylightCool : kDaylightWarm;
    const float inverse = 1.0F / temperature;
    const float x = (((((cubic[0] * inverse) + cubic[1]) * inverse) + cubic[2]) * inverse) + cubic[3];
    return { x, (((kDaylightY[0] * x) + kDaylightY[1]) * x) + kDaylightY[2] };
}

/**
 * The relative power of CIE daylight at a wavelength in nanometres and a correlated colour
 * temperature in kelvin. It is S0 + M1 S1 + M2 S2, with M1 and M2 rounded to three decimal places
 * as CIE 15 does. The components are interpolated linearly between their 5 nm rows, and the power
 * is zero outside them.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the wavelength first, as in every spectrum
MOYA_KERNEL inline float daylight(float lambda, float temperature) {
    const float row = (lambda - static_cast<float>(kDaylightFirst)) / static_cast<float>(kDaylightStep);
    if (!(row >= 0.0F) || row > static_cast<float>(kDaylightRows - 1)) {
        return 0.0F;
    }
    const glm::vec2 xy = daylightChromaticity(temperature);
    const auto weight = [&xy](const float * terms) { return terms[0] + (terms[1] * xy.x) + (terms[2] * xy.y); };
    const float denominator = weight(kDaylightWeights[0]);
    const float m1 = std::round(weight(kDaylightWeights[1]) / denominator * kDaylightRounding) / kDaylightRounding;
    const float m2 = std::round(weight(kDaylightWeights[2]) / denominator * kDaylightRounding) / kDaylightRounding;
    const int below = row >= static_cast<float>(kDaylightRows - 1) ? kDaylightRows - 2 : static_cast<int>(row);
    const float t = row - static_cast<float>(below);
    float component[3];
    for (int k = 0; k < 3; ++k) {
        component[k] = kDaylight[below][k] + (t * (kDaylight[below + 1][k] - kDaylight[below][k]));
    }
    return component[0] + (m1 * component[1]) + (m2 * component[2]);
}

};  // namespace v3d::moya::kernel::spectrum
