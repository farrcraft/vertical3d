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
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

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

/**
 * The scatter stage at a shading point: a direction drawn from the BSDF, the throughput carried
 * along it, and the ray that leaves. False when the path ends there: at a depth limit, for a
 * sample the BSDF refused, or by Russian roulette.
 *
 * From the settings' roulette depth on, a path survives with the chance of its largest throughput,
 * held to one, and a survivor's throughput is divided by that chance.
 **/
template <geometry::Traversal T>
MOYA_KERNEL bool scatter(const Paths<T> & paths, const surface::ShadingPoint & point, PathState & state) {
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (bounce >= paths.settings.maxDepth) {
        return false;
    }
    const geometry::Ray & arrived = MOYA_PATH(state, ray);
    const bsdf::Bsdf scattering = bsdf::atSurface(paths.surfaces, point, paths.table, MOYA_PATH(state, wavelengths));
    const glm::vec3 outgoing = -glm::normalize(arrived.direction);
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const bsdf::Sample drawn = scattering.sample(outgoing, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kScatter)));
    if (!drawn.found() || !counted(paths.settings, drawn.lobe, state)) {
        return false;
    }
    MOYA_PATH(state, throughput) *= drawn.value * (std::fabs(glm::dot(drawn.incoming, point.shading)) / drawn.density);
    MOYA_PATH(state, density) = drawn.density;
    MOYA_PATH(state, bounces) = bounce + 1U;
    if (bounce + 1U >= paths.settings.rouletteDepth) {
        const float survival = std::fmin(1.0F, MOYA_PATH(state, throughput).maximum());
        if (!(paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kRoulette)) < survival)) {
            return false;
        }
        MOYA_PATH(state, throughput) /= survival;
    }

    // a ray leaving on the side the normal faces starts from where a smooth surface lies
    const bool outward = glm::dot(drawn.incoming, point.geometric) > 0.0F;
    geometry::Ray leaving;
    leaving.origin = geometry::offsetStart(outward ? point.leave : point.position, outward ? point.geometric : -point.geometric);
    leaving.direction = drawn.incoming;
    leaving.start = 0.0F;
    leaving.end = std::numeric_limits<float>::infinity();
    leaving.time = arrived.time;
    leaving.kind = geometry::kIndirectRay;
    MOYA_PATH(state, ray) = leaving;
    return true;
}

/**
 * One path, from the camera until it leaves the scene or ends. The intersect, shade, scatter and
 * accumulate stages run back to back. A path that leaves the scene gathers the environment, and
 * one that meets a light's shape gathers what the light emits towards it and ends there.
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
        const light::Light * met = paths.lights.at(hit);
        if (met != nullptr) {
            MOYA_PATH(state, radiance) +=
                MOYA_PATH(state, throughput) * light::Lights::emitted(*met, paths.surfaces, ray, hit.time, paths.table, MOYA_PATH(state, wavelengths));
            return;
        }
        const surface::ShadingPoint point = paths.surfaces.at(hit, ray);
        if (!point.found() || !scatter(paths, point, state)) {
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
