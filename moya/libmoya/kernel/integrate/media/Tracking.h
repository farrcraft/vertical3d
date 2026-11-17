/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Dimensions.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/LightSamples.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Hash.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Footprint.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Homogeneous.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/Phase.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate::media {

/** How far a hash is shifted down to leave the 24 bits a float in [0, 1) holds. **/
inline constexpr std::uint32_t kCollisionShift = 40;

/** One over two to the 24th, which turns 24 bits into a float in [0, 1). **/
inline constexpr float kCollisionStep = 1.0F / 16777216.0F;

/** What a path's tracking through the medium it is in does before the surface it next meets. **/
enum class Tracked : std::uint8_t {
    Reached,
    Scattered,
    Ended
};

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

/**
 * Tracks a path through the homogeneous medium it is in, along its ray up to a distance in the
 * ray's lengths, which is endless when the ray meets nothing. A path in no medium, or in one of
 * no coefficients, reaches the end unchanged.
 *
 * The distance to a collision is drawn from the medium's extinction at the wavelength trackedOf()
 * names, and a collision scatters with the chance of that wavelength's scattering over its
 * extinction, and is otherwise absorbed. Both numbers come from collisionDraw(), and collisions
 * counts the draws the bounce has made. Each wavelength is weighed by rescale(): by the
 * transmittance to the end for a path that reaches it, and by the scattering times the
 * transmittance to the collision for one that scatters. A medium whose coefficients differ by
 * wavelength then stays unbiased at every wavelength. The density that a light the path meets
 * next is weighed by grows by what rescale() returns.
 *
 * A path that is absorbed ends there, as does one whose scattering a depth limit refuses. One
 * that scatters takes a light sample and draws a direction from the medium's phase function by
 * the bounce's draws, as a bounce off a surface does, and counts a volume bounce. The point is
 * lit by the lights that light an instance of no links. Its cone spreads as a lobe of roughness
 * one spreads it. Russian roulette treats it as any bounce. It gives a denoiser nothing past it.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the count of draws
MOYA_KERNEL Tracked track(const Paths<T, X> & paths, PathState & state, float end, std::uint32_t & collisions) {
    const std::uint32_t at = MOYA_PATH(state, media).medium();
    if (at == surface::kNoMaterial || volume::empty(paths.surfaces.materials[at].medium)) {
        return Tracked::Reached;
    }
    const spectrum::SampledWavelengths & wavelengths = MOYA_PATH(state, wavelengths);
    const volume::Coefficients coefficients = volume::coefficientsOf(paths.table, paths.surfaces.materials[at].medium, wavelengths);
    const spectrum::SampledSpectrum extinction = coefficients.extinction();
    const geometry::Ray ray = MOYA_PATH(state, ray);
    const float length = glm::length(ray.direction);
    const float span = (end - ray.start) * length;
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    const int tracked = trackedOf(pixel + paths.sampler.origin, sample);
    const glm::vec2 u = collisionDraw(pixel + paths.sampler.origin, sample, bounce, collisions);
    ++collisions;
    const float distance = volume::freeFlight(extinction[tracked], u.x);
    spectrum::SampledSpectrum & throughput = MOYA_PATH(state, throughput);
    if (!(distance < span)) {
        MOYA_PATH(state, density) *= rescale(throughput, MOYA_PATH(state, chances), volume::transmittance(extinction, span));
        return throughput.black() ? Tracked::Ended : Tracked::Reached;
    }
    const spectrum::SampledSpectrum scattered = coefficients.scattered();
    if (!(u.y * extinction[tracked] < scattered[tracked])) {
        return Tracked::Ended;
    }
    if (bounce >= paths.settings.maxDepth || MOYA_PATH(state, volumeBounces) >= paths.settings.volumeDepth) {
        return Tracked::Ended;
    }
    rescale(throughput, MOYA_PATH(state, chances), scattered * volume::transmittance(extinction, distance));
    if (throughput.black()) {
        return Tracked::Ended;
    }
    const glm::vec3 forward = ray.direction / length;
    const float along = (ray.start * length) + distance;
    const glm::vec3 point = ray.origin + (forward * along);
    MOYA_PATH(state, linked) = 0U;
    MOYA_PATH(state, radiance) += throughput * directInMedium(paths, coefficients, point, forward, state);
    const volume::PhaseSample drawn = volume::phaseSample(coefficients, forward, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kScatter)),
        paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kScatterChoice)));
    if (!drawn.found()) {
        return Tracked::Ended;
    }
    ++MOYA_PATH(state, volumeBounces);
    throughput *= drawn.value / drawn.density;
    MOYA_PATH(state, density) = drawn.density;
    MOYA_PATH(state, specular) = 0U;
    MOYA_PATH(state, from) = point;
    MOYA_PATH(state, normal) = glm::vec3 { 0.0F };
    MOYA_PATH(state, featured) = 1U;
    MOYA_PATH(state, cone) = surface::bounced(MOYA_PATH(state, cone), MOYA_PATH(state, cone).widthAt(along), 0.0F, 1.0F, true);
    MOYA_PATH(state, bounces) = bounce + 1U;
    if (bounce + 1U >= paths.settings.rouletteDepth) {
        const float survival = std::fmin(1.0F, throughput.maximum());
        if (!(paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kRoulette)) < survival)) {
            return Tracked::Ended;
        }
        throughput /= survival;
    }
    geometry::Ray & leaving = MOYA_PATH(state, ray);
    leaving.origin = point;
    leaving.direction = drawn.incoming;
    leaving.start = 0.0F;
    leaving.end = std::numeric_limits<float>::infinity();
    leaving.kind = geometry::kIndirectRay;
    return Tracked::Scattered;
}

};  // namespace v3d::moya::kernel::integrate::media
