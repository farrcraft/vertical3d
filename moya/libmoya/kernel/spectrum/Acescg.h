/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Daylight.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::spectrum {

/**
 * ACEScg to XYZ, by rows: the matrix that the AP1 primaries and the ACES white point give, with
 * no chromatic adaptation.
 **/
MOYA_KERNEL_DATA inline constexpr float kAcescgToXyz[3][3] = {
    { 0.6624541811F, 0.1340042065F, 0.1561876870F },
    { 0.2722287168F, 0.6740817658F, 0.0536895174F },
    { -0.0055746495F, 0.0040607335F, 1.0103391003F },
};

/** XYZ to ACEScg, by rows: the inverse of kAcescgToXyz. **/
MOYA_KERNEL_DATA inline constexpr float kXyzToAcescg[3][3] = {
    { 1.6410233797F, -0.3248032942F, -0.2364246952F },
    { -0.6636628587F, 1.6153315917F, 0.0167563477F },
    { 0.0117218943F, -0.0082844420F, 0.9883948585F },
};

/**
 * The correlated colour temperature, in kelvin, of the daylight that is ACES white. CIE daylight
 * at it, integrated through the observer, has the ACES white point's chromaticity, (0.32168,
 * 0.33767), to five decimal places.
 **/
inline constexpr float kAcesWhiteTemperature = 6000.0F;

/** The scale that gives daylight at kAcesWhiteTemperature a luminance of one. **/
inline constexpr float kAcesWhiteScale = 0.0101444221F;

/** The product of a 3 by 3 matrix, by rows, and a vector. **/
MOYA_KERNEL inline glm::vec3 multiply(const float (&matrix)[3][3], const glm::vec3 & v) {
    return { (matrix[0][0] * v.x) + (matrix[0][1] * v.y) + (matrix[0][2] * v.z),
        (matrix[1][0] * v.x) + (matrix[1][1] * v.y) + (matrix[1][2] * v.z),
        (matrix[2][0] * v.x) + (matrix[2][1] * v.y) + (matrix[2][2] * v.z) };
}

/** An XYZ colour in ACEScg. **/
MOYA_KERNEL inline glm::vec3 acescgFromXyz(const glm::vec3 & xyz) {
    return multiply(kXyzToAcescg, xyz);
}

/** An ACEScg colour in XYZ. **/
MOYA_KERNEL inline glm::vec3 acescgToXyz(const glm::vec3 & rgb) {
    return multiply(kAcescgToXyz, rgb);
}

/**
 * ACES white's power at a wavelength in nanometres: CIE daylight at kAcesWhiteTemperature, scaled
 * to a luminance of one. Its XYZ is ACEScg's (1, 1, 1). An RGB light shines with it, and an RGB
 * reflectance is fitted under it.
 **/
MOYA_KERNEL inline float acesWhite(float lambda) {
    return daylight(lambda, kAcesWhiteTemperature) * kAcesWhiteScale;
}

/** ACES white at the wavelengths a path carries. **/
MOYA_KERNEL inline SampledSpectrum acesWhite(const SampledWavelengths & wavelengths) {
    SampledSpectrum out;
    for (int i = 0; i < kSamples; ++i) {
        out[i] = acesWhite(wavelengths.lambda[i]);
    }
    return out;
}

};  // namespace v3d::moya::kernel::spectrum
