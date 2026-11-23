/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cstdint>

namespace v3d::moya::kernel::bsdf::scattering {

/** A hair's lobes: R, TT, TRT, and every longer path inside the fibre as one. **/
inline constexpr std::uint32_t kHairLobes = 4;

/** The lobes that have an azimuthal distribution of their own: all but the last. **/
inline constexpr std::uint32_t kHairShaped = kHairLobes - 1U;

/** The root of pi over eight, which turns an azimuthal roughness into a logistic's scale. **/
inline constexpr float kLogisticScale = 0.626657069F;

/**
 * A hair fibre as one closure scatters by, at a path's wavelengths. Each tint scales a lobe, and
 * the last lobe takes TRT's. The absorption is a coefficient over the fibre's radius. Each lobe
 * has a longitudinal variance, and each but the last an azimuthal logistic's scale. Each lobe but
 * the last sees the outgoing direction turned along the fibre by the cuticle's tilt, by an angle
 * whose sine and cosine are held. The offset is where light meets the fibre across its width,
 * from -1 to 1.
 **/
struct Fibre {
    spectrum::SampledSpectrum tints[kHairShaped] { spectrum::SampledSpectrum(1.0F), spectrum::SampledSpectrum(1.0F), spectrum::SampledSpectrum(1.0F) };
    spectrum::SampledSpectrum absorption { 0.0F };
    float ior { 1.55F };  // NOLINT(readability-magic-numbers) - MaterialX's default
    float variance[kHairLobes] { 0.1F, 0.05F, 0.2F, 0.2F };  // NOLINT(readability-magic-numbers) - MaterialX's defaults
    float scale[kHairShaped] { 0.1F * kLogisticScale, 0.05F * kLogisticScale, 0.2F * kLogisticScale };  // NOLINT(readability-magic-numbers)
    float tiltSine[kHairShaped] { 0.0F, 0.0F, 0.0F };
    float tiltCosine[kHairShaped] { 1.0F, 1.0F, 1.0F };
    float offset { 0.0F };
};

};  // namespace v3d::moya::kernel::bsdf::scattering
