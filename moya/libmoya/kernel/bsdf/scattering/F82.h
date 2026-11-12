/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cmath>

namespace v3d::moya::kernel::bsdf::scattering {

/** The cosine at which the F82 tint is given: one seventh, an angle near 82 degrees. **/
inline constexpr float kTintCosine = 1.0F / 7.0F;

/**
 * A conductor's Fresnel factor as OpenPBR gives it: Schlick's from the reflectance at normal
 * incidence, less Hoffman's term that brings it to the tint times Schlick's at kTintCosine. Each
 * wavelength is its own, so a coloured metal's reflectance is its spectrum's.
 **/
struct F82 {
    spectrum::SampledSpectrum normal { 1.0F };
    spectrum::SampledSpectrum tint { 1.0F };

    /** Schlick's factor at a cosine, for a reflectance at normal incidence. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the reflectance, then the cosine
    MOYA_KERNEL static float schlick(float normal, float cosine) {
        const float rest = 1.0F - cosine;
        const float squared = rest * rest;
        return normal + ((1.0F - normal) * squared * squared * rest);
    }

    /** The factor at a cosine between zero and one, held to [0, 1]. **/
    MOYA_KERNEL spectrum::SampledSpectrum operator()(float cosine) const {
        const float clamped = std::fmin(std::fmax(cosine, 0.0F), 1.0F);
        const float rest = 1.0F - clamped;
        const float sixth = rest * rest * rest * rest * rest * rest;
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float value = schlick(normal[i], clamped) - (lowered(i) * clamped * sixth);
            out[i] = std::fmin(std::fmax(value, 0.0F), 1.0F);
        }
        return out;
    }

    /**
     * The factor's mean over the hemisphere, weighted by the cosine: twice its integral times the
     * cosine. Schlick's part gives the normal reflectance and a twenty-first of the rest, and
     * Hoffman's term takes away a 126th of its scale.
     **/
    MOYA_KERNEL spectrum::SampledSpectrum average() const {
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float value = normal[i] + ((1.0F - normal[i]) / 21.0F) - (lowered(i) / 126.0F);
            out[i] = std::fmin(std::fmax(value, 0.0F), 1.0F);
        }
        return out;
    }

 private:
    /** The scale of Hoffman's term at a wavelength. **/
    MOYA_KERNEL float lowered(int wavelength) const {
        const float rest = 1.0F - kTintCosine;
        const float sixth = rest * rest * rest * rest * rest * rest;
        return schlick(normal[wavelength], kTintCosine) * (1.0F - tint[wavelength]) / (kTintCosine * sixth);
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
