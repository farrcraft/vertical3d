/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/CieD65.h>
#include <moya/libmoya/kernel/spectrum/CieObserver.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Visible.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::spectrum {

/**
 * The integral of the observer's y function at 1 nm steps, which a luminance is divided by so
 * that a constant spectrum of one has a luminance of one.
 **/
inline constexpr float kYIntegral = 106.856917F;

/** The temperature illuminant A is defined at, in kelvin. **/
inline constexpr float kTemperatureA = 2848.0F;

/** The second radiation constant as illuminant A's definition gives it, in nanometre kelvins. **/
inline constexpr float kRadiationA = 1.435e7F;

/** The wavelength every CIE illuminant is normalised to 100 at, in nanometres. **/
inline constexpr float kNormal = 560.0F;

/** The power illuminant A's definition raises a wavelength ratio to. **/
inline constexpr float kPowerA = 5.0F;

/**
 * The colour-matching functions x, y and z at a wavelength in nanometres, interpolated linearly
 * between the observer's 1 nm rows, and zero outside its range.
 **/
MOYA_KERNEL inline glm::vec3 observer(float lambda) {
    const float row = lambda - static_cast<float>(kObserverFirst);
    if (!(row >= 0.0F) || row > static_cast<float>(kObserverRows - 1)) {
        return glm::vec3(0.0F);
    }
    const int below = row >= static_cast<float>(kObserverRows - 1) ? kObserverRows - 2 : static_cast<int>(row);
    const float t = row - static_cast<float>(below);
    const float * low = kObserver[below];
    const float * high = kObserver[below + 1];
    return glm::vec3(low[0] + (t * (high[0] - low[0])), low[1] + (t * (high[1] - low[1])), low[2] + (t * (high[2] - low[2])));
}

/**
 * CIE standard illuminant D65's relative power at a wavelength in nanometres, interpolated
 * linearly between its 1 nm values, and zero outside its range.
 **/
MOYA_KERNEL inline float d65(float lambda) {
    const float row = lambda - static_cast<float>(kD65First);
    if (!(row >= 0.0F) || row > static_cast<float>(kD65Rows - 1)) {
        return 0.0F;
    }
    const int below = row >= static_cast<float>(kD65Rows - 1) ? kD65Rows - 2 : static_cast<int>(row);
    const float t = row - static_cast<float>(below);
    return kD65[below] + (t * (kD65[below + 1] - kD65[below]));
}

/**
 * CIE standard illuminant A's relative power at a wavelength in nanometres, from the formula that
 * defines it: a Planckian radiator at kTemperatureA, normalised to 100 at kNormal.
 **/
MOYA_KERNEL inline float illuminantA(float lambda) {
    if (!(lambda > 0.0F)) {
        return 0.0F;
    }
    const float ratio = std::pow(kNormal / lambda, kPowerA);
    const float normal = std::expm1(kRadiationA / (kTemperatureA * kNormal));
    return 100.0F * ratio * normal / std::expm1(kRadiationA / (kTemperatureA * lambda));
}

/** D65 at the wavelengths a path carries. **/
MOYA_KERNEL inline SampledSpectrum d65(const SampledWavelengths & wavelengths) {
    SampledSpectrum out;
    for (int i = 0; i < kSamples; ++i) {
        out[i] = d65(wavelengths.lambda[i]);
    }
    return out;
}

/** Illuminant A at the wavelengths a path carries. **/
MOYA_KERNEL inline SampledSpectrum illuminantA(const SampledWavelengths & wavelengths) {
    SampledSpectrum out;
    for (int i = 0; i < kSamples; ++i) {
        out[i] = illuminantA(wavelengths.lambda[i]);
    }
    return out;
}

/**
 * One sample's estimate of a spectrum's XYZ, from its values at the wavelengths a path carries.
 * Each wavelength's contribution is divided by the density it was drawn with, and the sum by
 * kYIntegral. The mean of many samples converges to the spectrum's XYZ, and a constant spectrum of
 * one has a Y of one. A wavelength drawn with no density contributes nothing.
 **/
MOYA_KERNEL inline glm::vec3 toXyz(const SampledSpectrum & spectrum, const SampledWavelengths & wavelengths) {
    glm::vec3 sum(0.0F);
    for (int i = 0; i < kSamples; ++i) {
        if (wavelengths.pdf[i] != 0.0F) {
            sum += observer(wavelengths.lambda[i]) * (spectrum[i] / wavelengths.pdf[i]);
        }
    }
    return sum / (static_cast<float>(kSamples) * kYIntegral);
}

};  // namespace v3d::moya::kernel::spectrum
