/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/media/Crossings.h>
#include <moya/libmoya/kernel/integrate/media/Enclosure.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Readings.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Volume.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate::media {

/** The primitive of a collision's hit. No reading takes it, and it makes the hit found. **/
inline constexpr std::uint32_t kCollisionPrimitive = 0;

/**
 * What picks the medium a collision names: the share of the joined extinction that the closed
 * surface's medium holds, a number below the joined extinction, and the wavelength tracked.
 **/
struct Pick {
    float aroundShare { 0.0F };
    float chosen { 0.0F };
    int tracked { 0 };
};

/**
 * A collision a distance along a ray, in the ray's lengths, as a hit that names a medium by its
 * outermost and innermost instances. It has no primitive of its own, so what only a surface has
 * reads as it does where nothing is met.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the outermost, then the innermost
MOYA_KERNEL inline geometry::Hit collisionHit(float distance, std::uint32_t asset, std::uint32_t object) {
    geometry::Hit out;
    out.distance = distance;
    out.primitive = kCollisionPrimitive;
    out.instances[0] = asset;
    out.instances[1] = object;
    out.depth = asset == object ? 1U : 2U;
    return out;
}

/** A collision in the medium a closed surface encloses, as collisionHit() makes it. **/
MOYA_KERNEL inline geometry::Hit collisionHit(float distance, const Enclosure & enclosure) {
    return collisionHit(distance, enclosure.asset, enclosure.object);
}

/** A collision in a volume, as collisionHit() makes it. **/
MOYA_KERNEL inline geometry::Hit collisionHit(float distance, const volume::Volume & placed) {
    const std::uint32_t depth = placed.depth < geometry::kInstanceDepth ? placed.depth : geometry::kInstanceDepth;
    return collisionHit(distance, placed.chain[0], depth != 0U ? placed.chain[depth - 1U] : placed.chain[0]);
}

/**
 * Adds the albedo and the normal a denoiser reads at a collision in a medium of some coefficients,
 * when the path has given none yet. The albedo is the medium's single-scattering albedo, its
 * scattering over its extinction, times the path's throughput, under ACES white. The normal points
 * back along the ray. The path gives nothing past it.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the direction, then the distance
MOYA_KERNEL void featuresInMedium(const Paths<T, X> & paths, PathState & state, const volume::Coefficients & here, const glm::vec3 & forward, float along) {
    if (MOYA_PATH(state, featured) != 0U || paths.features.size == 0U) {
        return;
    }
    MOYA_PATH(state, featured) = 1U;
    const spectrum::SampledWavelengths & wavelengths = MOYA_PATH(state, wavelengths);
    const spectrum::SampledSpectrum single = spectrum::safeDivide(here.scattered(), here.extinction());
    const spectrum::SampledSpectrum albedo = MOYA_PATH(state, throughput) * single * spectrum::acesWhite(wavelengths);
    const glm::vec3 rgb = spectrum::acescgFromXyz(spectrum::toXyz(albedo, wavelengths));
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    readFeatures(paths.pixels, Item { .x = pixel.x, .y = pixel.y }, MOYA_PATH(state, weight), paths.features, along, rgb, -forward);
}

/**
 * Records a collision in a medium, before the path scatters or is absorbed there. A camera ray's
 * collision is its first hit, which the hit names, and covers its sample: its alpha is one, and
 * its record holds the medium's instances and material at the collision's depth. Any path that has
 * given a denoiser nothing gives it the medium's albedo there, as featuresInMedium() says.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the material, the direction, the distance
MOYA_KERNEL void collided(const Paths<T, X> & paths, PathState & state, const geometry::Hit & hit, std::uint32_t material, const volume::Coefficients & here,
    const glm::vec3 & forward, float along) {
    if (MOYA_PATH(state, bounces) == 0U) {
        MOYA_PATH(state, coverage) = 1.0F;
        firstHit(paths, state, hit, MOYA_PATH(state, ray), surface::ShadingPoint {});
        cover(paths, state, hit, material, hit.distance, true);
    }
    featuresInMedium(paths, state, here, forward, along);
}

/**
 * Records a collision among the media at a point of a ray, a parameter t along it, as collided()
 * says. The pick's chosen, which lies below the joined extinction at the tracked wavelength, picks
 * the medium it names. The closed surface's medium the path is in holds the first share of that
 * extinction, as much as its own. Each volume holds a share as large as its own after it. A medium
 * is then named with the chance its extinction is of the whole.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void collidedAmong(const Paths<T, X> & paths, PathState & state, const Crossings & crossings, const spectrum::SampledSpectrum * extinctions,
    const Pick & pick, float t, const volume::Coefficients & here, const glm::vec3 & forward, float along) {
    const Enclosure * inside = MOYA_PATH(state, media).current();
    if (inside != nullptr && pick.chosen < pick.aroundShare) {
        collided(paths, state, collisionHit(t, *inside), inside->material, here, forward, along);
        return;
    }
    float below = pick.aroundShare;
    std::uint32_t picked = crossings.count;
    for (std::uint32_t i = 0; i < crossings.count && i < kMostCrossed; ++i) {
        if (extinctions[i][pick.tracked] > 0.0F) {
            picked = i;
            below += extinctions[i][pick.tracked];
            if (pick.chosen < below) {
                break;
            }
        }
    }
    if (picked >= crossings.count || picked >= kMostCrossed) {
        featuresInMedium(paths, state, here, forward, along);
        return;
    }
    const volume::Volume & placed = paths.volumes.volumes[crossings.entries[picked].volume];
    collided(paths, state, collisionHit(t, placed), placed.material, here, forward, along);
}

};  // namespace v3d::moya::kernel::integrate::media
