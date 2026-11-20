/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Hash.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::integrate::media {

/** How far a hash is shifted down to leave the 24 bits a float in [0, 1) holds. **/
inline constexpr std::uint32_t kCollisionShift = 40;

/** One over two to the 24th, which turns 24 bits into a float in [0, 1). **/
inline constexpr float kCollisionStep = 1.0F / 16777216.0F;

/**
 * The count a shadow ray's collisions start from, among a bounce's, so they take numbers apart
 * from the path's own.
 **/
inline constexpr std::uint32_t kShadowCollisions = 1U << 31U;

/** The most collisions one stretch of a ray is tracked through. **/
inline constexpr std::uint32_t kMostCollisions = 1U << 16U;

/**
 * The two numbers a collision in a medium draws, each in [0, 1): one for its distance and one
 * for what happens there. They are a hash of the pixel in the image, the sample, the bounce and
 * the collision's count within the bounce. A path's collisions take numbers apart from each
 * other and from every other path's, and the same ones on every run.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the sample, the bounce, then the collision
MOYA_KERNEL inline glm::vec2 collisionDraw(const glm::uvec2 & pixel, std::uint32_t sample, std::uint32_t bounce, std::uint32_t collision) {
    std::uint64_t value = sample::mix((static_cast<std::uint64_t>(pixel.y) << 32U) | pixel.x);
    value = sample::mix(value ^ ((static_cast<std::uint64_t>(sample) << 32U) | bounce));
    value = sample::mix(value ^ collision);
    const std::uint64_t second = sample::mix(value);
    return glm::vec2 { static_cast<float>(value >> kCollisionShift) * kCollisionStep, static_cast<float>(second >> kCollisionShift) * kCollisionStep };
}

/**
 * The wavelength a path is tracked by through media. A spectral path's are drawn as one evenly
 * spaced set, any of which is equally likely to come first, so it is tracked by its first. An
 * RGB path carries the same three channels every time, so it is tracked by one drawn from a
 * hash of its pixel in the image and its sample.
 **/
MOYA_KERNEL inline int trackedOf([[maybe_unused]] const glm::uvec2 & pixel, [[maybe_unused]] std::uint32_t sample) {
#ifdef MOYA_RGB
    const std::uint64_t value = sample::mix(sample::mix((static_cast<std::uint64_t>(pixel.y) << 32U) | pixel.x) ^ sample);
    return static_cast<int>((value >> kCollisionShift) % static_cast<std::uint64_t>(spectrum::kSamples));
#else
    return 0;
#endif
}

/**
 * Weighs a path by an event tracked at one of its wavelengths, by spectral multiple importance
 * sampling, and gives the factor its density grows by.
 *
 * factor is the event's value at each wavelength, which is also the density with which a path
 * tracked at that wavelength would have made it. chances are the densities of the path so far
 * had each wavelength been the one tracked, over a common scale, with an average of one. The
 * path's throughput is its value over the average of those densities, so it is multiplied by
 * the factor and divided by the average of the chances times the factor. That average is what is
 * returned, and the chances are rescaled to an average of one again. A path that no wavelength
 * could have made carries nothing.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the throughput, then the chances
MOYA_KERNEL inline float rescale(spectrum::SampledSpectrum & throughput, spectrum::SampledSpectrum & chances, const spectrum::SampledSpectrum & factor) {
    const spectrum::SampledSpectrum grown = chances * factor;
    const float average = grown.average();
    if (!(average > 0.0F)) {
        throughput = spectrum::SampledSpectrum(0.0F);
        return 0.0F;
    }
    throughput *= factor;
    throughput /= average;
    chances = grown / average;
    return average;
}

/** What is left of a majorant over an extinction at each wavelength, or none where it is short. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the majorant, then the extinction
MOYA_KERNEL inline spectrum::SampledSpectrum leftOf(const spectrum::SampledSpectrum & majorant, const spectrum::SampledSpectrum & extinction) {
    spectrum::SampledSpectrum out = majorant - extinction;
    for (int i = 0; i < spectrum::kSamples; ++i) {
        out[i] = out[i] > 0.0F ? out[i] : 0.0F;
    }
    return out;
}

/**
 * Takes the factor by which a shadow ray would have tracked a path into chances of the shadow
 * ray's, rescaled to an average of one again, and gives their average before they were rescaled.
 * Chances that no wavelength could have made give zero, and are left as they were.
 **/
MOYA_KERNEL inline float lighten(spectrum::SampledSpectrum & chances, const spectrum::SampledSpectrum & factor) {
    const spectrum::SampledSpectrum grown = chances * factor;
    const float average = grown.average();
    if (average > 0.0F) {
        chances = grown / average;
    }
    return average;
}

};  // namespace v3d::moya::kernel::integrate::media
