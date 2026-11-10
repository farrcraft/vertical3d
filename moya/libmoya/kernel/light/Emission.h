/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>

namespace v3d::moya::kernel::light {

/** A value held to [low, high]. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the value, then its bounds
MOYA_KERNEL inline float heldTo(float value, float low, float high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

/** How far from zero to one a value has come between two edges, eased at both ends. **/
MOYA_KERNEL inline float smoothStep(float value, float from, float to) {
    if (!(to > from)) {
        return value < from ? 0.0F : 1.0F;
    }
    const float along = heldTo((value - from) / (to - from), 0.0F, 1.0F);
    // Hermite's cubic, 3t^2 - 2t^3
    constexpr float kSquared = 3.0F;
    return along * along * (kSquared - (2.0F * along));
}

/**
 * The share of a light's radiance its cone lets out at an angle off its axis, whose cosine is
 * given: one inside the cone, falling smoothly to zero at its edge, and zero past it.
 **/
MOYA_KERNEL inline float coneShare(const Light & light, float cosine) {
    if (light.coneAngle >= kQuarterTurn && !(light.coneSoftness > 0.0F)) {
        return 1.0F;
    }
    const float off = std::acos(heldTo(cosine, -1.0F, 1.0F));
    if (off > light.coneAngle) {
        return 0.0F;
    }
    return 1.0F - smoothStep(off, light.coneAngle * (1.0F - light.coneSoftness), light.coneAngle);
}

/** A light's spectrum at the wavelengths a path carries, before its scale and its cone. **/
MOYA_KERNEL inline spectrum::SampledSpectrum spectrumOf(const Light & light, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    if (!(light.temperature > 0.0F)) {
        return spectrum::illuminant(table, light.colour, wavelengths);
    }
    spectrum::SampledSpectrum out = spectrum::unbounded(table, light.colour, wavelengths);
    for (int i = 0; i < spectrum::kSamples; ++i) {
#ifdef MOYA_RGB
        out[i] *= light.planckColour[i];
#else
        out[i] *= spectrum::planck(wavelengths.lambda[i], light.temperature) * light.planckScale;
#endif
    }
    return out;
}

};  // namespace v3d::moya::kernel::light
