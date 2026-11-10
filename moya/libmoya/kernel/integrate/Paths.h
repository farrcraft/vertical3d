/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/AtSurface.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/FilterSample.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Offset.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Dimensions.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Sampling.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/** How far short of a light's point a shadow ray stops, as a share of its length. **/
inline constexpr float kShadowShort = 1.0e-4F;

/**
 * What the integrator's kernel function reads: the film, the pixel filter, the camera, the
 * sampler, a traversal, the scene's surfaces and lights, the RGB-to-spectrum table and the
 * settings. A pass
 * adds the samples from first, count of them, to each pixel.
 **/
template <geometry::Traversal T>
struct Paths {
    film::Pixels pixels;
    film::PixelFilter filter;
    camera::Camera camera;
    sample::ZSobol sampler;
    T traversal;
    surface::Surfaces surfaces;
    light::Lights lights;
    spectrum::RgbTable table;
    Settings settings;
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
};

/**
 * The generate stage: a camera sample's state. Its place in the pixel is drawn from the pixel
 * filter, with the filter's weight, and its point on the lens, its time and its wavelengths are
 * drawn after it. It carries a throughput of one and has gathered no light.
 **/
template <geometry::Traversal T>
MOYA_KERNEL void generate(const Paths<T> & paths, const glm::uvec2 & pixel, std::uint32_t sample, PathState & state) {
    const film::FilterSample placed = paths.filter.sample(paths.sampler.get2D(pixel, sample, kFilter));
    const glm::vec2 raster = glm::vec2 { static_cast<float>(pixel.x), static_cast<float>(pixel.y) } + 0.5F + placed.offset;
    MOYA_PATH(state, pixel) = pixel;
    MOYA_PATH(state, sample) = sample;
    MOYA_PATH(state, ray) = paths.camera.ray(raster, paths.sampler.get2D(pixel, sample, kLens), paths.sampler.get1D(pixel, sample, kTime));
    MOYA_PATH(state, wavelengths) = spectrum::SampledWavelengths::visible(paths.sampler.get1D(pixel, sample, kWavelengths));
    MOYA_PATH(state, weight) = placed.weight;
    MOYA_PATH(state, throughput) = spectrum::SampledSpectrum(1.0F);
    MOYA_PATH(state, radiance) = spectrum::SampledSpectrum(0.0F);
}

/** Whether a bounce of a kind stays within that kind's depth limit, and counts it. **/
MOYA_KERNEL inline bool counted(const Settings & settings, bsdf::Lobe lobe, PathState & state) {
    std::uint32_t * count = &MOYA_PATH(state, diffuseBounces);
    std::uint32_t limit = settings.diffuseDepth;
    switch (lobe) {
        case bsdf::Lobe::Diffuse:
            break;
        case bsdf::Lobe::Glossy:
            count = &MOYA_PATH(state, glossyBounces);
            limit = settings.glossyDepth;
            break;
        case bsdf::Lobe::Specular:
            count = &MOYA_PATH(state, specularBounces);
            limit = settings.specularDepth;
            break;
        case bsdf::Lobe::Transmission:
            count = &MOYA_PATH(state, transmissionBounces);
            limit = settings.transmissionDepth;
            break;
    }
    if (*count >= limit) {
        return false;
    }
    ++*count;
    return true;
}

/** The power heuristic's weight for a strategy of one density against another. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the strategy's own, then the other's
MOYA_KERNEL inline float powerHeuristic(float own, float other) {
    const float ownSquared = own * own;
    const float sum = ownSquared + (other * other);
    return sum > 0.0F ? ownSquared / sum : 0.0F;
}

/**
 * The ray that leaves a shading point along a direction. One that leaves on the side the
 * geometric normal faces starts from where a smooth surface lies, and each starts off the surface.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the direction, then the time
MOYA_KERNEL inline geometry::Ray leavingRay(const surface::ShadingPoint & point, const glm::vec3 & direction, float time, std::uint32_t kind) {
    const bool outward = glm::dot(direction, point.geometric) > 0.0F;
    geometry::Ray out;
    out.origin = geometry::offsetStart(outward ? point.leave : point.position, outward ? point.geometric : -point.geometric);
    out.direction = direction;
    out.start = 0.0F;
    out.end = std::numeric_limits<float>::infinity();
    out.time = time;
    out.kind = kind;
    return out;
}

/**
 * Next event estimation at a shading point: the light a light sample brings in, weighed against
 * the BSDF's sample by the power heuristic. A light is drawn in proportion to its power, then a
 * point on it, and a shadow ray towards the point says whether anything lies between. A bounce
 * that a depth limit refuses samples no light, since the BSDF's sample there ends the path.
 **/
template <geometry::Traversal T>
MOYA_KERNEL spectrum::SampledSpectrum direct(const Paths<T> & paths, const surface::ShadingPoint & point, const bsdf::Bsdf & scattering,
    const glm::vec3 & outgoing, const PathState & state) {
    const spectrum::SampledSpectrum none(0.0F);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (paths.settings.strategy == Strategy::Bsdf || bounce >= paths.settings.maxDepth || MOYA_PATH(state, diffuseBounces) >= paths.settings.diffuseDepth) {
        return none;
    }
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t place = paths.lights.pick(paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kLightPick)));
    const float chance = paths.lights.chance(place);
    if (!(chance > 0.0F)) {
        return none;
    }
    const light::Light & chosen = paths.lights.lights[place];
    const float time = MOYA_PATH(state, ray).time;
    glm::mat4 toWorld { 1.0F };
    if (!paths.surfaces.placement(chosen.id, paths.surfaces.held(time), toWorld)) {
        return none;
    }
    const light::LightSample drawn = light::sample(chosen, toWorld, point.position, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kLightPoint)));
    if (!drawn.found()) {
        return none;
    }
    const spectrum::SampledSpectrum value = scattering.evaluate(outgoing, drawn.incoming);
    if (value.black()) {
        return none;
    }
    const spectrum::SampledSpectrum emitted = light::radiance(chosen, toWorld, drawn.point, -drawn.incoming, paths.table, MOYA_PATH(state, wavelengths));
    if (emitted.black()) {
        return none;
    }
    geometry::Ray shadow = leavingRay(point, drawn.incoming, time, geometry::kShadowRay);
    shadow.direction = drawn.point - shadow.origin;
    shadow.end = 1.0F - kShadowShort;
    if (paths.traversal.any(shadow)) {
        return none;
    }
    const float density = chance * drawn.density;
    const float weight = paths.settings.strategy == Strategy::Mis ? powerHeuristic(density, scattering.density(outgoing, drawn.incoming)) : 1.0F;
    return value * emitted * (std::fabs(glm::dot(drawn.incoming, point.shading)) * weight / density);
}

/**
 * The scatter stage at a shading point: a direction drawn from the BSDF, the throughput carried
 * along it, and the ray that leaves. False when the path ends there: at a depth limit, for a
 * sample the BSDF refused, or by Russian roulette.
 *
 * From the settings' roulette depth on, a path survives with the chance of its largest throughput,
 * held to one, and a survivor's throughput is divided by that chance.
 **/
template <geometry::Traversal T>
MOYA_KERNEL bool scatter(const Paths<T> & paths, const surface::ShadingPoint & point, const bsdf::Bsdf & scattering, const glm::vec3 & outgoing,
    PathState & state) {
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (bounce >= paths.settings.maxDepth) {
        return false;
    }
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const bsdf::Sample drawn = scattering.sample(outgoing, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kScatter)));
    if (!drawn.found() || !counted(paths.settings, drawn.lobe, state)) {
        return false;
    }
    MOYA_PATH(state, throughput) *= drawn.value * (std::fabs(glm::dot(drawn.incoming, point.shading)) / drawn.density);
    MOYA_PATH(state, density) = drawn.density;
    MOYA_PATH(state, from) = point.position;
    MOYA_PATH(state, bounces) = bounce + 1U;
    if (bounce + 1U >= paths.settings.rouletteDepth) {
        const float survival = std::fmin(1.0F, MOYA_PATH(state, throughput).maximum());
        if (!(paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kRoulette)) < survival)) {
            return false;
        }
        MOYA_PATH(state, throughput) /= survival;
    }
    MOYA_PATH(state, ray) = leavingRay(point, drawn.incoming, MOYA_PATH(state, ray).time, geometry::kIndirectRay);
    return true;
}

/**
 * The share of a light that a ray met carries back. A camera ray's carries all of it, since
 * nothing else samples the light for it. A bounce's carries the power heuristic's weight of the
 * BSDF's sample against the light's: all of it when the BSDF alone finds lights, and none when
 * light samples alone do.
 **/
template <geometry::Traversal T>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the light, then the ray
MOYA_KERNEL float metShare(const Paths<T> & paths, std::uint32_t place, const geometry::Ray & ray, const geometry::Hit & hit, const PathState & state) {
    if (MOYA_PATH(state, bounces) == 0U || paths.settings.strategy == Strategy::Bsdf) {
        return 1.0F;
    }
    if (paths.settings.strategy == Strategy::Light) {
        return 0.0F;
    }
    const light::Light & met = paths.lights.lights[place];
    glm::mat4 toWorld { 1.0F };
    if (!paths.surfaces.placement(met.id, paths.surfaces.held(ray.time), toWorld)) {
        return 1.0F;
    }
    const float density = paths.lights.chance(place) * light::density(met, toWorld, MOYA_PATH(state, from), ray.at(hit.distance));
    return powerHeuristic(MOYA_PATH(state, density), density);
}

/**
 * One path, from the camera until it leaves the scene or ends. The intersect, shade, scatter and
 * accumulate stages run back to back. At each shading point a path gathers a light sample before
 * it scatters. A path that leaves the scene gathers the environment, and one that meets a light's
 * shape gathers its share of what the light emits towards it, and ends there.
 **/
template <geometry::Traversal T>
MOYA_KERNEL void trace(const Paths<T> & paths, PathState & state) {
    for (;;) {
        const geometry::Ray & ray = MOYA_PATH(state, ray);
        const geometry::Hit hit = paths.traversal.nearest(ray);
        if (!hit.found()) {
            const spectrum::SampledSpectrum environment = spectrum::illuminant(paths.table, paths.settings.environment, MOYA_PATH(state, wavelengths));
            MOYA_PATH(state, radiance) += MOYA_PATH(state, throughput) * environment;
            return;
        }
        const std::uint32_t met = paths.lights.at(hit);
        if (met != geometry::kNoPrimitive) {
            const float share = metShare(paths, met, ray, hit, state);
            if (share > 0.0F) {
                MOYA_PATH(state, radiance) +=
                    MOYA_PATH(state, throughput) * paths.lights.emitted(met, paths.surfaces, ray, hit, paths.table, MOYA_PATH(state, wavelengths)) * share;
            }
            return;
        }
        const surface::ShadingPoint point = paths.surfaces.at(hit, ray);
        if (!point.found()) {
            return;
        }
        const bsdf::Bsdf scattering = bsdf::atSurface(paths.surfaces, point, paths.table, MOYA_PATH(state, wavelengths));
        const glm::vec3 outgoing = -glm::normalize(ray.direction);
        MOYA_PATH(state, radiance) += MOYA_PATH(state, throughput) * direct(paths, point, scattering, outgoing, state);
        if (!scatter(paths, point, scattering, outgoing, state)) {
            return;
        }
    }
}

/** One pixel of a pass: each of its samples traced, and added to the pixel in index order. **/
template <geometry::Traversal T>
MOYA_KERNEL void paths(const Paths<T> & params, Item item) {
    const glm::uvec2 pixel { item.x, item.y };
    for (std::uint32_t sample = params.first; sample < params.first + params.count; ++sample) {
        PathState state;
        generate(params, pixel, sample, state);
        trace(params, state);
        params.pixels.add(item, MOYA_PATH(state, weight), MOYA_PATH(state, radiance), MOYA_PATH(state, wavelengths));
    }
}

};  // namespace v3d::moya::kernel::integrate
