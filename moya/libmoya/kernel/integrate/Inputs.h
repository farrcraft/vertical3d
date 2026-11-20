/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/integrate/Expressions.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/kernel/integrate/media/Enclosure.h>
#include <moya/libmoya/kernel/integrate/media/Media.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Readings.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/integrate/Trail.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Volumes.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/** How far short of a light's point a shadow ray stops, as a share of its length. **/
inline constexpr float kShadowShort = 1.0e-4F;

/**
 * The share of a surface's estimated albedo its singular closures hold, at or above which a path
 * passes the surface by for a denoiser's albedo and normal.
 **/
inline constexpr float kFeaturedShare = 0.5F;

/**
 * The most surfaces a ray passes. A shadow ray from a light with a shadow link is blocked past
 * them. A path ends past them, and a camera ray stops looking for the media it starts in.
 **/
inline constexpr std::uint32_t kMostPassed = 256;

/** The most closed surfaces a camera ray tells apart as it finds the media it starts in. **/
inline constexpr std::uint32_t kMostSeen = 2U * media::kMediaDepth;

/**
 * What the integrator's kernel function reads: the film, the pixel filter, the camera, the
 * sampler, a traversal and a texture access. Then the scene's surfaces, volumes and lights, the
 * RGB-to-spectrum table, the albedo table that energy compensation reads, and the settings. A
 * pass adds the samples from first, count of them, to each pixel. firstHits names the channels
 * each camera ray fills where it first meets the scene, and features those a denoiser reads.
 * trail holds kTrailSteps steps for each pixel, row by row, where sample zero records its events,
 * or is empty to record none. expressions are the render's light path expressions. covered holds
 * count records for each pixel, row by row, where each camera sample of the pass records what it
 * covers, or is empty to record none.
 **/
template <geometry::Traversal T, texture::Texels X>
struct Paths {
    film::Pixels pixels;
    film::PixelFilter filter;
    camera::Camera camera;
    sample::ZSobol sampler;
    T traversal;
    X textures;
    surface::Surfaces surfaces;
    volume::Volumes volumes;
    light::Lights lights;
    spectrum::RgbTable table;
    bsdf::Albedos albedos;
    Settings settings;
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
    Array<FirstHit> firstHits;
    Array<FirstHit> features;
    Array<Step> trail;
    Expressions expressions;
    Array<Covered> covered;
};

/** The first of a pixel's records of what its samples cover in a pass. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL std::uint32_t coveredFrom(const Paths<T, X> & paths, std::uint32_t x, std::uint32_t y) {
    return ((y * paths.pixels.width) + x) * paths.count;
}

/**
 * Records what a camera ray covers where it first meets the scene: how far along it met anything,
 * and the instances of a surface it met and the surface's material. A ray that met no surface, or
 * met a light's shape, covers nothing. A collision in a medium is recorded as collided, its hit
 * naming the medium's instances. A later ray records nothing.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the material, then the distance
MOYA_KERNEL void cover(const Paths<T, X> & paths, const PathState & state, const geometry::Hit & hit, std::uint32_t material, float distance,
    bool collided = false) {
    if (paths.covered.size == 0U || MOYA_PATH(state, bounces) != 0U) {
        return;
    }
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    Covered out;
    out.weight = MOYA_PATH(state, weight);
    out.depth = distance;
    out.collided = collided ? 1U : 0U;
    if (hit.found() && hit.depth != 0U) {
        out.object = hit.instances[hit.depth - 1U];
        out.asset = hit.instances[0];
        out.material = material == surface::kNoMaterial ? kUncovered : material;
    }
    paths.covered[coveredFrom(paths, pixel.x, pixel.y) + (MOYA_PATH(state, sample) - paths.first)] = out;
}

/**
 * Records an event of a path in its pixel's trail, after the events before it, when the path is
 * its pixel's sample zero and the trail has room. gathered is whether the path gathers the
 * light of the event's light there, rather than going on from it.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void record(const Paths<T, X> & paths, const PathState & state, const Event & event, bool gathered) {
    if (paths.trail.size == 0U || MOYA_PATH(state, sample) != 0U) {
        return;
    }
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    const std::uint32_t first = ((pixel.y * paths.pixels.width) + pixel.x) * kTrailSteps;
    for (std::uint32_t i = first; i < first + kTrailSteps; ++i) {
        if (paths.trail[i].used == 0U) {
            paths.trail[i] = Step { .event = event, .used = 1U, .gathered = gathered ? 1U : 0U };
            return;
        }
    }
}

/** Records an event the path goes on from, and steps its expressions' state by it. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void advance(const Paths<T, X> & paths, PathState & state, const Event & event) {
    record(paths, state, event, false);
    if (!paths.expressions.none()) {
        MOYA_PATH(state, expression) = paths.expressions.step(MOYA_PATH(state, expression), event);
    }
}

/**
 * Adds light a path gathers, already times its throughput, to the channel of each expression a
 * state of the expressions' automaton accepts, in ACEScg, with the sample's filter weight.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void accepted(const Paths<T, X> & paths, const PathState & state, std::uint32_t at, const spectrum::SampledSpectrum & value) {
    const std::uint32_t flags = paths.expressions.accepts[at];
    if (flags == 0U) {
        return;
    }
    const glm::vec3 rgb = spectrum::acescgFromXyz(spectrum::toXyz(value, MOYA_PATH(state, wavelengths)));
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    for (std::uint32_t i = 0; i < paths.expressions.channels.size; ++i) {
        if ((flags & (1U << i)) != 0U) {
            const Expressions::Channel & channel = paths.expressions.channels[i];
            paths.pixels.keep(Item { .x = pixel.x, .y = pixel.y }, channel.offset, channel.combine, MOYA_PATH(state, weight), 0.0F, &rgb.x, 3U);
        }
    }
}

/**
 * Light a path gathers from a light it meets, already times its throughput, recorded by the
 * light's event and added to each expression that matches the path's events with it. The path's
 * own state is left as it is.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void gathered(const Paths<T, X> & paths, const PathState & state, const Event & light, const spectrum::SampledSpectrum & value) {
    record(paths, state, light, true);
    if (!paths.expressions.none()) {
        accepted(paths, state, paths.expressions.step(MOYA_PATH(state, expression), light), value);
    }
}

/**
 * Light a light sample gathers through one closure, already times its throughput: recorded by
 * the closure's bounce and then the light's event, and added to each expression that matches the
 * path's events with both.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the closure's bounce, then the light
MOYA_KERNEL void gathered(const Paths<T, X> & paths, const PathState & state, const Event & bounce, const Event & light, const spectrum::SampledSpectrum & value) {
    record(paths, state, bounce, true);
    record(paths, state, light, true);
    if (!paths.expressions.none()) {
        accepted(paths, state, paths.expressions.step(paths.expressions.step(MOYA_PATH(state, expression), bounce), light), value);
    }
}

/**
 * Adds the albedo and the shading normal a denoiser reads, at the first surface a path meets whose
 * singular closures hold less than kFeaturedShare of its albedo. A surface it reaches through
 * mirrors and smooth glass alone gives its albedo times what they let through, which is the
 * path's throughput. The albedo is the surface's closures that are not singular, under ACES
 * white, so a white surface's is one.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void features(const Paths<T, X> & paths, PathState & state, const bsdf::Bsdf & scattering, const surface::ShadingPoint & point,
    float depth) {
    if (MOYA_PATH(state, featured) != 0U || paths.features.size == 0U || scattering.singularShare() >= kFeaturedShare) {
        return;
    }
    MOYA_PATH(state, featured) = 1U;
    const spectrum::SampledWavelengths & wavelengths = MOYA_PATH(state, wavelengths);
    const spectrum::SampledSpectrum albedo = MOYA_PATH(state, throughput) * scattering.spreadAlbedo() * spectrum::acesWhite(wavelengths);
    const glm::vec3 rgb = spectrum::acescgFromXyz(spectrum::toXyz(albedo, wavelengths));
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    readFeatures(paths.pixels, Item { .x = pixel.x, .y = pixel.y }, MOYA_PATH(state, weight), paths.features, depth, rgb, point.shading);
}

/** Adds a camera ray's first hit to the channels that read it, and nothing for a later ray. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void firstHit(const Paths<T, X> & paths, const PathState & state, const geometry::Hit & hit, const geometry::Ray & ray,
    const surface::ShadingPoint & point) {
    if (MOYA_PATH(state, bounces) != 0U || paths.firstHits.size == 0U) {
        return;
    }
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    readFirstHit(paths.pixels, Item { .x = pixel.x, .y = pixel.y }, MOYA_PATH(state, weight), paths.firstHits, paths.camera, paths.surfaces, hit, ray, point);
}

/**
 * The closed surfaces a camera ray starts inside, found by tracing it on through everything it
 * meets. A ray is inside each surface whose first face it meets is a back face, and enters them
 * innermost last. It looks at most kMostPassed hits, and tells apart at most kMostSeen surfaces.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL media::Media around(const Paths<T, X> & paths, geometry::Ray probe) {
    media::Enclosure inside[media::kMediaDepth];
    std::uint32_t found = 0;
    std::uint32_t seen[kMostSeen] {};
    std::uint32_t known = 0;
    const geometry::EveryHit every;
    for (std::uint32_t passed = 0; passed < kMostPassed; ++passed) {
        const geometry::Hit hit = paths.traversal.nearest(probe, every);
        if (!hit.found()) {
            break;
        }
        const surface::ShadingPoint point = paths.surfaces.at(hit, probe);
        if (point.found() && point.material != surface::kNoMaterial && paths.surfaces.materials[point.material].encloses()) {
            const std::uint32_t key = media::keyOf(hit);
            bool met = false;
            for (std::uint32_t i = 0; i < known; ++i) {
                met = met || seen[i] == key;
            }
            if (!met && known < kMostSeen) {
                seen[known] = key;
                ++known;
                if (!point.front && found < media::kMediaDepth) {
                    inside[found] = media::enclosureOf(paths.surfaces, hit, point.material);
                    ++found;
                }
            }
        }
        probe.start = std::fmax(hit.distance * (1.0F + kShadowShort), hit.distance + std::numeric_limits<float>::min());
    }
    media::Media out;
    for (std::uint32_t i = found; i > 0U; --i) {
        out.enter(inside[i - 1U]);
    }
    return out;
}

};  // namespace v3d::moya::kernel::integrate
