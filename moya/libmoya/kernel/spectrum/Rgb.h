/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/RgbSigmoid.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::spectrum {

/**
 * The reflectance of an ACEScg colour at the wavelengths a path carries. The colour is clamped to
 * [0, 1], and its spectrum lies in [0, 1]. A grey of zero or one gives exactly zero or one. In an
 * RGB build it is the clamped colour.
 **/
MOYA_KERNEL inline SampledSpectrum reflectance([[maybe_unused]] const RgbTable & table, const glm::vec3 & colour,
    [[maybe_unused]] const SampledWavelengths & wavelengths) {
    SampledSpectrum out;
#ifdef MOYA_RGB
    for (int i = 0; i < kSamples; ++i) {
        out[i] = RgbTable::clamp(colour[i]);
    }
#else
    const RgbSigmoid sigmoid = table(colour);
    for (int i = 0; i < kSamples; ++i) {
        out[i] = sigmoid(wavelengths.lambda[i]);
    }
#endif
    return out;
}

/**
 * An ACEScg colour that may exceed one, as a spectrum: the reflectance of colour / (2 * largest),
 * where largest is its largest channel, times 2 * largest. Its value at any wavelength is at most
 * twice the largest channel. A colour with no positive channel gives zero. In an RGB build it is
 * the colour with no channel below zero.
 **/
MOYA_KERNEL inline SampledSpectrum unbounded([[maybe_unused]] const RgbTable & table, const glm::vec3 & colour,
    [[maybe_unused]] const SampledWavelengths & wavelengths) {
#ifdef MOYA_RGB
    SampledSpectrum out;
    for (int i = 0; i < kSamples; ++i) {
        out[i] = colour[i] > 0.0F ? colour[i] : 0.0F;
    }
    return out;
#else
    float largest = colour.x > colour.y ? colour.x : colour.y;
    largest = colour.z > largest ? colour.z : largest;
    if (!(largest > 0.0F)) {
        return SampledSpectrum();
    }
    const float scale = 2.0F * largest;
    return reflectance(table, colour / scale, wavelengths) * scale;
#endif
}

/**
 * The light an ACEScg colour names, at the wavelengths a path carries: its unbounded spectrum
 * times ACES white. A light of colour (1, 1, 1) shines with ACES white, and its XYZ is ACEScg's
 * (1, 1, 1).
 **/
MOYA_KERNEL inline SampledSpectrum illuminant(const RgbTable & table, const glm::vec3 & colour, const SampledWavelengths & wavelengths) {
    return unbounded(table, colour, wavelengths) * acesWhite(wavelengths);
}

};  // namespace v3d::moya::kernel::spectrum
