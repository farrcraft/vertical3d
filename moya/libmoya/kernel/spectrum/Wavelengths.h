/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Visible.h>

namespace v3d::moya::kernel::spectrum {

/**
 * The wavelengths one path carries, and the density each was drawn with.
 *
 * The first is the hero wavelength, drawn from visiblePdf. The others are drawn at the same point
 * of the distribution offset by equal steps, wrapped, so the four lie evenly through it and one
 * random number places them all.
 *
 * An RGB build draws no wavelengths. Each lambda is zero and each density one, so an estimate
 * divided by its density is the estimate.
 **/
struct SampledWavelengths {
    float lambda[kSamples] {};
    float pdf[kSamples] {};

    /** The wavelengths a path carries for a random number u in [0, 1). **/
    MOYA_KERNEL static SampledWavelengths visible([[maybe_unused]] float u) {
        SampledWavelengths out;
#ifdef MOYA_RGB
        for (float & density : out.pdf) {
            density = 1.0F;
        }
#else
        for (int i = 0; i < kSamples; ++i) {
            float offset = u + (static_cast<float>(i) / static_cast<float>(kSamples));
            if (offset >= 1.0F) {
                offset -= 1.0F;
            }
            out.lambda[i] = sampleVisible(offset);
            out.pdf[i] = visiblePdf(out.lambda[i]);
        }
#endif
        return out;
    }

    /** The density of each wavelength, as a spectrum, to divide a path's estimate by. **/
    MOYA_KERNEL SampledSpectrum density() const {
        SampledSpectrum out;
        for (int i = 0; i < kSamples; ++i) {
            out[i] = pdf[i];
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::spectrum
