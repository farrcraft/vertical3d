/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
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
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/kernel/integrate/Dimensions.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/integrate/Expressions.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/kernel/integrate/Keeping.h>
#include <moya/libmoya/kernel/integrate/Opacity.h>
#include <moya/libmoya/kernel/integrate/Passing.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Readings.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/integrate/Trail.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/light/Dome.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Sampling.h>
#include <moya/libmoya/kernel/light/SphericalTriangle.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Footprint.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>

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
 * The share of a surface's estimated albedo its singular closures hold, at or above which a path
 * passes the surface by for a denoiser's albedo and normal.
 **/
inline constexpr float kFeaturedShare = 0.5F;

/** The most surfaces a shadow ray from a light with a shadow link meets before it is blocked. **/
inline constexpr std::uint32_t kMostPassed = 256;

/**
 * What the integrator's kernel function reads: the film, the pixel filter, the camera, the
 * sampler, a traversal and a texture access. Then the scene's surfaces and lights, the
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
 * met a light's shape, covers nothing. A later ray records nothing.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the material, then the distance
MOYA_KERNEL void cover(const Paths<T, X> & paths, const PathState & state, const geometry::Hit & hit, std::uint32_t material, float distance) {
    if (paths.covered.size == 0U || MOYA_PATH(state, bounces) != 0U) {
        return;
    }
    const glm::uvec2 & pixel = MOYA_PATH(state, pixel);
    Covered out;
    out.weight = MOYA_PATH(state, weight);
    out.depth = distance;
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
 * The generate stage: a camera sample's state. Its place in the pixel is drawn from the pixel
 * filter, with the filter's weight, and its point on the lens, its time and its wavelengths are
 * drawn after it. It carries a throughput of one and has gathered no light, and its cone is the
 * camera's for its ray. Its first event is the camera.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void generate(const Paths<T, X> & paths, const glm::uvec2 & pixel, std::uint32_t sample, PathState & state) {
    const film::FilterSample placed = paths.filter.sample(paths.sampler.get2D(pixel, sample, kFilter));
    // a pixel is the film's own, and the camera sees it at its place in the image
    const glm::uvec2 imaged = pixel + paths.sampler.origin;
    const glm::vec2 raster = glm::vec2 { static_cast<float>(imaged.x), static_cast<float>(imaged.y) } + 0.5F + placed.offset;
    MOYA_PATH(state, pixel) = pixel;
    MOYA_PATH(state, sample) = sample;
    MOYA_PATH(state, ray) = paths.camera.ray(raster, paths.sampler.get2D(pixel, sample, kLens), paths.sampler.get1D(pixel, sample, kTime));
    MOYA_PATH(state, cone) = paths.camera.cone(MOYA_PATH(state, ray));
    MOYA_PATH(state, wavelengths) = spectrum::SampledWavelengths::visible(paths.sampler.get1D(pixel, sample, kWavelengths));
    MOYA_PATH(state, weight) = placed.weight;
    MOYA_PATH(state, throughput) = spectrum::SampledSpectrum(1.0F);
    MOYA_PATH(state, radiance) = spectrum::SampledSpectrum(0.0F);
    MOYA_PATH(state, expression) = 0U;
    advance(paths, state, Event {});
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
 * The light a light sample draws at a shading point of the path's set of linked lights, at a time
 * within the shutter. The set's light tree draws it, or the power of every light does, with a mesh
 * light's triangle then drawn in proportion to its area. Its chance holds the triangle's. A light
 * drawn by power that does not light the set draws nothing.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL light::Picked pickLight(const Paths<T, X> & paths, const surface::ShadingPoint & point, float time, const PathState & state) {
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    const float u = paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kLightPick));
    const std::uint32_t set = MOYA_PATH(state, linked);
    if (paths.settings.picking == Picking::Tree) {
        return paths.lights.pickByTree(paths.surfaces, set, point.position, point.shading, time, u);
    }
    light::Picked out { .place = paths.lights.pick(u) };
    if (out.place == geometry::kNoPrimitive || !paths.lights.links.lights(set, out.place)) {
        return light::Picked {};
    }
    out.chance = paths.lights.chance(out.place);
    if (out.chance > 0.0F && paths.lights.lights[out.place].shape == light::Shape::Mesh) {
        out.primitive = paths.lights.pickTriangle(out.place, paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kLightTriangle)));
        out.chance *= paths.lights.triangleChance(out.place, out.primitive);
    }
    return out;
}

/**
 * The chance that pickLight() draws the light at a place, and the triangle given of a mesh light.
 * It is drawn at a shading point of a set of linked lights with a normal, at a time within the
 * shutter.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, the place, then the time
MOYA_KERNEL float pickChance(const Paths<T, X> & paths, std::uint32_t set, std::uint32_t place, std::uint32_t primitive, const glm::vec3 & from,
    const glm::vec3 & normal, float time) {  // NOLINT(bugprone-easily-swappable-parameters)
    if (paths.settings.picking == Picking::Tree) {
        return paths.lights.chanceByTree(paths.surfaces, set, place, primitive, from, normal, time);
    }
    const float chance = paths.lights.chance(place);
    return paths.lights.lights[place].shape == light::Shape::Mesh ? chance * paths.lights.triangleChance(place, primitive) : chance;
}

/**
 * A point drawn on the light at a place from a point, at a time within the shutter. A mesh light's
 * point is drawn on the triangle given. A dome light is drawn by its image's compensated way under
 * multiple importance sampling, and by its other way under light samples alone, where nothing else
 * finds what compensation leaves out.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, the triangle, then the time
MOYA_KERNEL light::LightSample drawLight(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t primitive, const glm::vec3 & from, float time,
    const glm::vec2 & u) {  // NOLINT(bugprone-easily-swappable-parameters)
    const light::Light & chosen = paths.lights.lights[place];
    if (chosen.shape == light::Shape::Dome) {
        return light::domeSample(paths.lights, place, paths.lights.placement(place, time), from, u, paths.settings.strategy == Strategy::Mis);
    }
    if (chosen.shape != light::Shape::Mesh) {
        return light::sample(chosen, paths.lights.placement(place, time), from, u);
    }
    glm::vec3 corners[3];
    glm::vec3 normal { 0.0F };
    if (primitive == geometry::kNoPrimitive || !paths.lights.triangleAt(paths.surfaces, place, primitive, time, corners, normal)) {
        return light::LightSample {};
    }
    light::faceTowards(chosen.twoSided, corners, from, normal);
    return light::triangleSample(corners, normal, from, u);
}

/**
 * The density with which drawLight() draws a point of the light at a place from a point, at a
 * time within the shutter, under multiple importance sampling. A mesh light's point lies on one of
 * its triangles, which a hit names, and its density is on that triangle.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the triangle
MOYA_KERNEL float lightDensity(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t primitive, const glm::vec3 & from, const glm::vec3 & met,
    float time) {  // NOLINT(bugprone-easily-swappable-parameters)
    const light::Light & light = paths.lights.lights[place];
    if (light.shape == light::Shape::Dome) {
        return light::domeDensity(paths.lights, place, paths.lights.placement(place, time), glm::normalize(met - from), true);
    }
    if (light.shape != light::Shape::Mesh) {
        return light::density(light, paths.lights.placement(place, time), from, met);
    }
    glm::vec3 corners[3];
    glm::vec3 normal { 0.0F };
    if (!paths.lights.triangleAt(paths.surfaces, place, primitive, time, corners, normal)) {
        return 0.0F;
    }
    light::faceTowards(light.twoSided, corners, from, normal);
    return light::triangleDensity(corners, normal, from, met);
}

/** What the light at a place sends to a point from a point drawn on it, at a time. **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the time
MOYA_KERNEL spectrum::SampledSpectrum arrivingFrom(const Paths<T, X> & paths, std::uint32_t place, float time, const glm::vec3 & from,
    const light::LightSample & drawn, const spectrum::SampledWavelengths & wavelengths) {
    const light::Light & chosen = paths.lights.lights[place];
    if (chosen.shape == light::Shape::Mesh) {
        return light::glow(chosen, paths.table, wavelengths);
    }
    const glm::mat4 toWorld = paths.lights.placement(place, time);
    if (chosen.shape == light::Shape::Dome) {
        return light::domeRadiance(paths.lights, place, toWorld, drawn.incoming, paths.table, wavelengths);
    }
    return light::arriving(chosen, toWorld, from, drawn.point, drawn.incoming, paths.lights.tint(place, toWorld, drawn.point), paths.table, wavelengths) *
        paths.lights.shaping(place, toWorld, -drawn.incoming);
}

/**
 * The share of a light that passes along a shadow ray, past everything that casts its shadows.
 * Each surface it meets lets one minus its opacity through, so a solid one lets none through.
 *
 * A ray from a light whose shadow link leaves instances out passes through each of those, from
 * just past where it met it. One that meets more than kMostPassed surfaces, left out or cut out,
 * is taken as blocked.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL float passing(const Paths<T, X> & paths, const light::Light & light, geometry::Ray shadow) {
    if (!light.shadowLinked) {
        Passing<X> filter { .surfaces = paths.surfaces, .textures = paths.textures, .ray = shadow };
        return paths.traversal.any(shadow, filter) ? 0.0F : filter.passing;
    }
    geometry::EveryHit every;
    float through = 1.0F;
    for (std::uint32_t passed = 0; passed < kMostPassed; ++passed) {
        const geometry::Hit hit = paths.traversal.nearest(shadow, every);
        if (!hit.found()) {
            return through;
        }
        if (paths.lights.links.casts(light, hit)) {
            through *= 1.0F - opacityAt(paths.surfaces, paths.textures, hit, shadow);
            if (!(through > 0.0F)) {
                return 0.0F;
            }
        }
        shadow.start = std::fmax(hit.distance * (1.0F + kShadowShort), hit.distance + std::numeric_limits<float>::min());
    }
    return 0.0F;
}

/**
 * Next event estimation at a shading point: the light a light sample brings in, weighed against
 * the BSDF's sample by the power heuristic. A light is drawn by pickLight(), then a point on it,
 * and a shadow ray towards the point says how much of its light passes what lies between. A
 * bounce that a depth limit refuses samples no light, since the BSDF's sample there ends the path.
 *
 * A light that only a light sample finds takes the whole weight, and is sampled whatever the
 * strategy. It lies at one point or along one direction, or its shadow link leaves an instance
 * out. While the BSDF alone finds the lights, a light sample of any other light gathers nothing.
 *
 * The light is gathered closure by closure: each closure's projected value towards the light,
 * weighed against that closure's own density. Each closure's light is gathered after its bounce's
 * event, so expressions split it by lobe.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL spectrum::SampledSpectrum direct(const Paths<T, X> & paths, const surface::ShadingPoint & point, const bsdf::Bsdf & scattering,
    const glm::vec3 & outgoing, const PathState & state) {
    const spectrum::SampledSpectrum none(0.0F);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (bounce >= paths.settings.maxDepth || MOYA_PATH(state, diffuseBounces) >= paths.settings.diffuseDepth) {
        return none;
    }
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const float time = MOYA_PATH(state, ray).time;
    const float held = paths.surfaces.held(time);
    const light::Picked picked = pickLight(paths, point, held, state);
    const float chance = picked.chance;
    if (!(chance > 0.0F)) {
        return none;
    }
    const std::uint32_t place = picked.place;
    const light::Light & chosen = paths.lights.lights[place];
    const bool alone = light::sampledAlone(chosen);
    if (paths.settings.strategy == Strategy::Bsdf && !alone) {
        return none;
    }
    const light::LightSample drawn =
        drawLight(paths, place, picked.primitive, point.position, held, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kLightPoint)));
    if (!drawn.found()) {
        return none;
    }
    bsdf::Bsdf::Part parts[bsdf::kMostClosures];
    bool any = false;
    for (std::uint32_t i = 0; i < scattering.count && i < bsdf::kMostClosures; ++i) {
        parts[i] = scattering.part(i, outgoing, drawn.incoming);
        any = any || !parts[i].projected.black();
    }
    if (!any) {
        return none;
    }
    const spectrum::SampledSpectrum emitted = arrivingFrom(paths, place, held, point.position, drawn, MOYA_PATH(state, wavelengths));
    if (emitted.black()) {
        return none;
    }
    // a shadow ray towards a distant or a dome light runs on past everything in the scene
    geometry::Ray shadow = leavingRay(point, drawn.incoming, time, geometry::kShadowRay);
    if (chosen.shape != light::Shape::Distant && chosen.shape != light::Shape::Dome) {
        shadow.direction = drawn.point - shadow.origin;
        shadow.end = 1.0F - kShadowShort;
    }
    const float through = passing(paths, chosen, shadow);
    if (!(through > 0.0F)) {
        return none;
    }
    const float density = chance * drawn.density;
    const bool weighed = paths.settings.strategy == Strategy::Mis && !alone;
    spectrum::SampledSpectrum out(0.0F);
    for (std::uint32_t i = 0; i < scattering.count && i < bsdf::kMostClosures; ++i) {
        const bsdf::Bsdf::Part & part = parts[i];
        if (part.projected.black()) {
            continue;
        }
        const float weight = weighed ? powerHeuristic(density, part.density) : 1.0F;
        const spectrum::SampledSpectrum piece = part.projected * emitted * (through * weight / density);
        out += piece;
        gathered(paths, state, scatteredBy(part.lobe, false, part.diffuse), litBy(chosen), MOYA_PATH(state, throughput) * piece);
    }
    return out;
}

/**
 * The scatter stage at a shading point: a direction drawn from the BSDF, the throughput carried
 * along it, and the ray that leaves. False when the path ends there: at a depth limit, for a
 * sample the BSDF refused, or by Russian roulette.
 *
 * The throughput is multiplied by the sample's projected value over its density. A specular
 * sample's holds no cosine, and the path marks the sample specular. A bounce within its depth
 * limit records its event, whether or not Russian roulette ends the path after it.
 *
 * The cone that leaves starts as wide as the footprint it arrived with, and spreads as the
 * surface's curvature and the sampled lobe's roughness spread it.
 *
 * From the settings' roulette depth on, a path survives with the chance of its largest throughput,
 * held to one, and a survivor's throughput is divided by that chance.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL bool scatter(const Paths<T, X> & paths, const surface::ShadingPoint & point, const bsdf::Bsdf & scattering, const glm::vec3 & outgoing,
    PathState & state) {
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (bounce >= paths.settings.maxDepth) {
        return false;
    }
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const bsdf::Sample drawn = scattering.sample(outgoing, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kScatter)),
        paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kScatterChoice)));
    if (!drawn.found() || !counted(paths.settings, drawn.lobe, state)) {
        return false;
    }
    advance(paths, state, scatteredBy(drawn));
    MOYA_PATH(state, throughput) *= drawn.projected / drawn.density;
    MOYA_PATH(state, density) = drawn.density;
    MOYA_PATH(state, specular) = drawn.specular ? 1U : 0U;
    MOYA_PATH(state, from) = point.position;
    MOYA_PATH(state, normal) = point.shading;
    const bool crossed = drawn.lobe == bsdf::Lobe::Transmission;
    MOYA_PATH(state, cone) =
        surface::bounced(MOYA_PATH(state, cone), point.footprint.width(), crossed ? 0.0F : paths.surfaces.curvature(point), drawn.roughness, crossed);
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
 * The share of a light that a ray met carries back, at a point of the light's shape, a point of a
 * mesh light's triangle, or a point along the ray to a distant light. A camera ray's carries all
 * of it, since nothing else samples the light for it, but none of a mesh light's unless camera
 * rays see it. A bounce's carries none of a light that does not light the set of linked lights of
 * the shading point it left, or that only a light sample finds. A specular bounce's carries all
 * of it, whatever the strategy, since no light sample could have drawn its direction. Otherwise
 * it carries the power heuristic's weight of the BSDF's sample against the light's: all of it
 * when the BSDF alone finds lights, and none when light samples alone do.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the light, the triangle, then the time
MOYA_KERNEL float metShare(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t primitive, float time, const glm::vec3 & met,
    const PathState & state) {
    if (MOYA_PATH(state, bounces) == 0U) {
        const light::Light & light = paths.lights.lights[place];
        // a light with a shape is hidden from the camera by its instance, and a mesh light and a
        // dome by their own flag
        const bool flagged = light.shape == light::Shape::Mesh || light.shape == light::Shape::Dome;
        return !flagged || light.camera ? 1.0F : 0.0F;
    }
    const std::uint32_t set = MOYA_PATH(state, linked);
    if (!paths.lights.links.lights(set, place) || light::sampledAlone(paths.lights.lights[place])) {
        return 0.0F;
    }
    if (MOYA_PATH(state, specular) != 0U || paths.settings.strategy == Strategy::Bsdf) {
        return 1.0F;
    }
    if (paths.settings.strategy == Strategy::Light) {
        return 0.0F;
    }
    const glm::vec3 & from = MOYA_PATH(state, from);
    const float density = pickChance(paths, set, place, primitive, from, MOYA_PATH(state, normal), time) * lightDensity(paths, place, primitive, from, met, time);
    return powerHeuristic(MOYA_PATH(state, density), density);
}

/**
 * What a ray that leaves the scene meets, each light gathered by its share: every dome light, and
 * each distant light with an angle whose cone holds the ray. A camera ray sees the dome lights
 * and no distant light. Each light it gathers records its event.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL spectrum::SampledSpectrum leaving(const Paths<T, X> & paths, const geometry::Ray & ray, const PathState & state) {
    const spectrum::SampledWavelengths & wavelengths = MOYA_PATH(state, wavelengths);
    spectrum::SampledSpectrum out(0.0F);
    const float time = paths.surfaces.held(ray.time);
    const glm::vec3 towards = glm::normalize(ray.direction);
    for (std::uint32_t i = 0; i < paths.lights.domes.size; ++i) {
        const std::uint32_t place = paths.lights.domes[i];
        const float share = metShare(paths, place, geometry::kNoPrimitive, time, MOYA_PATH(state, from) + towards, state);
        if (share > 0.0F) {
            const spectrum::SampledSpectrum piece = light::domeRadiance(paths.lights, place, paths.lights.placement(place, time), towards, paths.table, wavelengths) * share;
            out += piece;
            gathered(paths, state, litBy(paths.lights.lights[place]), MOYA_PATH(state, throughput) * piece);
        }
    }
    if (MOYA_PATH(state, bounces) == 0U) {
        return out;
    }
    for (std::uint32_t i = 0; i < paths.lights.distant.size; ++i) {
        const std::uint32_t place = paths.lights.distant[i];
        const spectrum::SampledSpectrum emitted =
            light::fromAfar(paths.lights.lights[place], paths.lights.placement(place, time), towards, paths.table, wavelengths);
        if (emitted.black()) {
            continue;
        }
        const float share = metShare(paths, place, geometry::kNoPrimitive, time, MOYA_PATH(state, from) + towards, state);
        if (share > 0.0F) {
            const spectrum::SampledSpectrum piece = emitted * share;
            out += piece;
            gathered(paths, state, litBy(paths.lights.lights[place]), MOYA_PATH(state, throughput) * piece);
        }
    }
    return out;
}

/**
 * One path, from the camera until it leaves the scene or ends. The intersect, shade, scatter and
 * accumulate stages run back to back. Each shading point holds the footprint the path's cone
 * makes there. At each shading point a path gathers its share of what
 * each mesh light emits there, a mesh light's and its material's both. It then takes a light
 * sample of its set of linked lights, before it scatters. A path that leaves the scene gathers
 * the dome and distant lights it meets. One that meets a light's shape gathers its share of what
 * the light emits towards it, and ends there. A ray passes through a surface that is cut out
 * with a chance of one minus its opacity, drawn from the bounce's own draw. A camera ray that
 * meets anything covers its sample, and one that leaves the scene does not. Each light the path
 * gathers records its event, after the event of the bounce it arrived by.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void trace(const Paths<T, X> & paths, PathState & state) {
    for (;;) {
        const geometry::Ray & ray = MOYA_PATH(state, ray);
        const Keeping<X> filter { .surfaces = paths.surfaces,
            .textures = paths.textures,
            .ray = ray,
            .sampler = paths.sampler,
            .pixel = MOYA_PATH(state, pixel),
            .sample = MOYA_PATH(state, sample),
            .dimension = bounceDimension(MOYA_PATH(state, bounces), kOpacity) };
        const geometry::Hit hit = paths.traversal.nearest(ray, filter);
        if (!hit.found()) {
            firstHit(paths, state, hit, ray, surface::ShadingPoint {});
            cover(paths, state, hit, surface::kNoMaterial, std::numeric_limits<float>::infinity());
            MOYA_PATH(state, radiance) += MOYA_PATH(state, throughput) * leaving(paths, ray, state);
            return;
        }
        if (MOYA_PATH(state, bounces) == 0U) {
            MOYA_PATH(state, coverage) = 1.0F;
        }
        const std::uint32_t met = paths.lights.at(hit);
        if (met != geometry::kNoPrimitive) {
            firstHit(paths, state, hit, ray, surface::ShadingPoint {});
            cover(paths, state, geometry::Hit {}, surface::kNoMaterial, hit.distance);
            const float time = paths.surfaces.held(hit.time);
            const float share = metShare(paths, met, geometry::kNoPrimitive, time, ray.at(hit.distance), state);
            if (share > 0.0F) {
                const spectrum::SampledSpectrum lit =
                    MOYA_PATH(state, throughput) * paths.lights.emitted(met, time, ray, hit, paths.table, MOYA_PATH(state, wavelengths)) * share;
                MOYA_PATH(state, radiance) += lit;
                gathered(paths, state, litBy(paths.lights.lights[met]), lit);
            }
            return;
        }
        surface::ShadingPoint point = paths.surfaces.at(hit, ray);
        firstHit(paths, state, hit, ray, point);
        cover(paths, state, hit, point.material, hit.distance);
        if (!point.found()) {
            return;
        }
        // a hit's distance is in the ray's lengths, and the cone's in world units
        const float length = glm::length(ray.direction);
        point.footprint = surface::footprintOf(MOYA_PATH(state, cone).widthAt(hit.distance * length), ray.direction / length, point.shading);
        for (std::uint32_t glowing = paths.lights.emitting(hit); glowing != geometry::kNoPrimitive; glowing = paths.lights.emitting(hit, glowing)) {
            // a light seen from behind its triangles emits only when it is two-sided
            if (!point.front && !paths.lights.lights[glowing].twoSided) {
                continue;
            }
            const float share = metShare(paths, glowing, hit.primitive, paths.surfaces.held(hit.time), point.position, state);
            if (share > 0.0F) {
                const spectrum::SampledSpectrum lit = MOYA_PATH(state, throughput) * light::glow(paths.lights.lights[glowing], paths.table, MOYA_PATH(state, wavelengths)) * share;
                MOYA_PATH(state, radiance) += lit;
                gathered(paths, state, litBy(paths.lights.lights[glowing]), lit);
            }
        }
        MOYA_PATH(state, linked) = paths.lights.links.setOf(hit);
        const glm::vec3 outgoing = -glm::normalize(ray.direction);
        const bsdf::Bsdf scattering = bsdf::atSurface(paths.surfaces, paths.textures, point, paths.table, paths.albedos, MOYA_PATH(state, wavelengths), outgoing);
        features(paths, state, scattering, point, hit.distance * length);
        MOYA_PATH(state, radiance) += MOYA_PATH(state, throughput) * direct(paths, point, scattering, outgoing, state);
        if (!scatter(paths, point, scattering, outgoing, state)) {
            return;
        }
        // a path that leaves through a lobe that is not singular gives a denoiser nothing past it
        if (MOYA_PATH(state, specular) == 0U) {
            MOYA_PATH(state, featured) = 1U;
        }
    }
}

/**
 * One pixel of a pass: each of its samples traced, and added to the pixel in index order. The
 * first half of the pass's samples, which are stratified as a whole run is, go to the half beauty
 * too. A pixel that has stopped takes none, and its records of what samples cover are empty.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void paths(const Paths<T, X> & params, Item item) {
    if (params.pixels.done(item)) {
        for (std::uint32_t i = 0; params.covered.size != 0U && i < params.count; ++i) {
            params.covered[coveredFrom(params, item.x, item.y) + i] = Covered {};
        }
        return;
    }
    const glm::uvec2 pixel { item.x, item.y };
    for (std::uint32_t sample = params.first; sample < params.first + params.count; ++sample) {
        PathState state;
        generate(params, pixel, sample, state);
        trace(params, state);
        if (params.covered.size != 0U) {
            const glm::vec3 rgb = spectrum::acescgFromXyz(spectrum::toXyz(MOYA_PATH(state, radiance), MOYA_PATH(state, wavelengths)));
            Covered & covered = params.covered[coveredFrom(params, item.x, item.y) + (sample - params.first)];
            covered.red = rgb.x;
            covered.green = rgb.y;
            covered.blue = rgb.z;
        }
        const bool half = (sample - params.first) < (params.count / 2U);
        params.pixels.add(item, MOYA_PATH(state, weight), MOYA_PATH(state, radiance), MOYA_PATH(state, wavelengths), half, MOYA_PATH(state, coverage));
    }
}

};  // namespace v3d::moya::kernel::integrate
