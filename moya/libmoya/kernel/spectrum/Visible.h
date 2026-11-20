/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

namespace v3d::moya::kernel::spectrum {

/** The shortest wavelength a path carries, in nanometres. **/
inline constexpr float kShortest = 360.0F;

/** The longest wavelength a path carries, in nanometres. **/
inline constexpr float kLongest = 830.0F;

/** The wavelength visiblePdf peaks at, in nanometres. **/
inline constexpr float kPeak = 538.0F;

/** How fast visiblePdf falls away from its peak, per nanometre. **/
inline constexpr float kFalloff = 0.0072F;

/** visiblePdf's scale, which makes it integrate to one over the visible range. **/
inline constexpr float kPdfScale = 0.0039398042F;

/** tanh(kFalloff (kPeak - kShortest)): where the visible range starts on tanh's scale. **/
inline constexpr float kTanhShortest = 0.85691062F;

/** kTanhShortest + tanh(kFalloff (kLongest - kPeak)): the visible range's width on that scale. **/
inline constexpr float kTanhWidth = 1.82750197F;

/**
 * The density visible wavelengths are drawn with, at a wavelength in nanometres: proportional to
 * 1 / cosh^2(kFalloff (lambda - kPeak)) over the visible range, and zero outside it. It follows
 * the observer's luminance, so the wavelengths that affect a picture most are drawn most often.
 **/
MOYA_KERNEL inline float visiblePdf(float lambda) {
    if (lambda < kShortest || lambda > kLongest) {
        return 0.0F;
    }
    const float spread = std::cosh(kFalloff * (lambda - kPeak));
    return kPdfScale / (spread * spread);
}

/**
 * The wavelength, in nanometres, at which visiblePdf's distribution reaches u, for u in [0, 1].
 * Zero gives the shortest visible wavelength and one the longest.
 **/
MOYA_KERNEL inline float sampleVisible(float u) {
    // the density's integral is a tanh, so its inverse is an atanh, scaled to [0, 1] over the range
    const float lambda = kPeak - (std::atanh(kTanhShortest - (kTanhWidth * u)) / kFalloff);
    if (lambda < kShortest) {
        return kShortest;
    }
    if (lambda > kLongest) {
        return kLongest;
    }
    return lambda;
}

};  // namespace v3d::moya::kernel::spectrum
