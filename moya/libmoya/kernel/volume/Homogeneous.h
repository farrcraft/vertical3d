/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cmath>
#include <limits>

namespace v3d::moya::kernel::volume {

/**
 * The share of light that passes a distance through a homogeneous medium of an extinction, at
 * each wavelength, by Beer and Lambert's law. Over an endless distance a wavelength of no
 * extinction passes whole and any other is lost.
 **/
MOYA_KERNEL inline spectrum::SampledSpectrum transmittance(const spectrum::SampledSpectrum & extinction, float distance) {
    spectrum::SampledSpectrum out;
    for (int i = 0; i < spectrum::kSamples; ++i) {
        const float optical = extinction[i] > 0.0F ? extinction[i] * distance : 0.0F;
        out[i] = std::exp(-optical);
    }
    return out;
}

/**
 * The distance to a path's next collision in a homogeneous medium of an extinction, drawn from
 * the exponential distribution of that extinction by a number in [0, 1). A medium of no
 * extinction gives an endless distance.
 **/
MOYA_KERNEL inline float freeFlight(float extinction, float u) {
    if (!(extinction > 0.0F)) {
        return std::numeric_limits<float>::infinity();
    }
    return -std::log1p(-u) / extinction;
}

};  // namespace v3d::moya::kernel::volume
