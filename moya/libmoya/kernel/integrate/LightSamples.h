/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Offset.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Dimensions.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/media/Enclosure.h>
#include <moya/libmoya/kernel/integrate/media/Media.h>
#include <moya/libmoya/kernel/integrate/Opacity.h>
#include <moya/libmoya/kernel/integrate/Passing.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/light/Dome.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Picked.h>
#include <moya/libmoya/kernel/light/Sampling.h>
#include <moya/libmoya/kernel/light/SphericalTriangle.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Homogeneous.h>
#include <moya/libmoya/kernel/volume/Medium.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/**
 * What passes along a shadow ray. share is what the surfaces it meets let through, and through
 * what the media it crosses let through at each wavelength. chance is the density with which a
 * path tracked through those media would have passed them all. A ray that crosses no medium lets
 * through one at each wavelength, with a chance of one.
 **/
struct Passed {
    float share { 0.0F };
    spectrum::SampledSpectrum through { 1.0F };
    float chance { 1.0F };
};

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
 * Whether a ray inside media passes by the surface a hit met. It passes a boundary, and a surface
 * that encloses a medium of lower priority than the medium it is in. A surface it passes it
 * enters or leaves, as it crosses it.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL bool passesBy(const Paths<T, X> & paths, media::Media & media, const geometry::Hit & hit, const surface::ShadingPoint & point) {
    if (!point.found() || point.material == surface::kNoMaterial) {
        return false;
    }
    const surface::Material & material = paths.surfaces.materials[point.material];
    if (!material.encloses()) {
        return false;
    }
    const media::Enclosure crossed = media::enclosureOf(paths.surfaces, hit, point.material);
    if (material.boundary == 0U && !media.outranked(crossed.priority)) {
        return false;
    }
    media.cross(crossed, point.front);
    return true;
}

/**
 * Takes the transmittance of the medium a path is in over a distance into what passes along a
 * shadow ray. tracked are the path's chances, times the transmittance of each medium so far.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the media, then the distance
MOYA_KERNEL void attenuate(const Paths<T, X> & paths, const media::Media & media, float distance, const spectrum::SampledWavelengths & wavelengths, Passed & out,
    spectrum::SampledSpectrum & tracked) {
    const std::uint32_t at = media.medium();
    if (at == surface::kNoMaterial || volume::empty(paths.surfaces.materials[at].medium)) {
        return;
    }
    const volume::Coefficients coefficients = volume::coefficientsOf(paths.table, paths.surfaces.materials[at].medium, wavelengths);
    const spectrum::SampledSpectrum passes = volume::transmittance(coefficients.extinction(), distance);
    out.through *= passes;
    tracked *= passes;
}

/**
 * The light that passes along a shadow ray, past everything that casts its shadows. Each surface
 * it meets lets one minus its opacity through, so a solid one lets none through.
 *
 * A ray from a light whose shadow link leaves instances out passes through each of those, from
 * just past where it met it. One that meets more than kMostPassed surfaces, left out or cut out,
 * is taken as blocked.
 *
 * Where any material encloses a medium, the ray carries a copy of the media of the path it
 * leaves, and passes by the surfaces passedBy() names, entering and leaving their media as a path
 * does. Each medium between lets through its transmittance at each wavelength, by Beer and
 * Lambert's law.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL Passed passing(const Paths<T, X> & paths, const light::Light & light, geometry::Ray shadow, const PathState & state) {
    Passed out;
    if (paths.surfaces.media == 0U && !light.shadowLinked) {
        Passing<X> filter { .surfaces = paths.surfaces, .textures = paths.textures, .ray = shadow };
        out.share = paths.traversal.any(shadow, filter) ? 0.0F : filter.passing;
        return out;
    }
    media::Media media = MOYA_PATH(state, media);
    spectrum::SampledSpectrum tracked = MOYA_PATH(state, chances);
    const float length = glm::length(shadow.direction);
    geometry::EveryHit every;
    float through = 1.0F;
    for (std::uint32_t passed = 0; passed < kMostPassed; ++passed) {
        const geometry::Hit hit = paths.traversal.nearest(shadow, every);
        if (paths.surfaces.media != 0U) {
            attenuate(paths, media, ((hit.found() ? hit.distance : shadow.end) - shadow.start) * length, MOYA_PATH(state, wavelengths), out, tracked);
            if (out.through.black()) {
                return Passed {};
            }
        }
        if (!hit.found()) {
            out.share = through;
            out.chance = tracked.average();
            return out;
        }
        // a surface the ray passes by casts no shadow
        const bool passes = paths.surfaces.media != 0U && passesBy(paths, media, hit, paths.surfaces.at(hit, shadow));
        if (!passes && paths.lights.links.casts(light, hit)) {
            through *= 1.0F - opacityAt(paths.surfaces, paths.textures, hit, shadow);
            if (!(through > 0.0F)) {
                return Passed {};
            }
        }
        shadow.start = std::fmax(hit.distance * (1.0F + kShadowShort), hit.distance + std::numeric_limits<float>::min());
    }
    return Passed {};
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
 *
 * Light that crosses media is multiplied by their transmittance, and the closure's density is
 * multiplied by the chance a tracked path would have passed them, as passing() gives both.
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
    const Passed passed = passing(paths, chosen, shadow, state);
    if (!(passed.share > 0.0F)) {
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
        const float weight = weighed ? powerHeuristic(density, part.density * passed.chance) : 1.0F;
        const spectrum::SampledSpectrum piece = part.projected * emitted * (passed.share * weight / density) * passed.through;
        out += piece;
        gathered(paths, state, scatteredBy(part.lobe, false, part.diffuse), litBy(chosen), MOYA_PATH(state, throughput) * piece);
    }
    return out;
}

/**
 * Next event estimation at a point in a medium: the light a light sample brings in, scattered by
 * the medium's phase function towards where the path came from. It is weighed against the phase
 * function's sample by the power heuristic, as direct() weighs a BSDF's. A point in a medium has
 * no normal, so a light is picked as for a point that faces every way. The shadow ray starts at
 * the point itself.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the direction
MOYA_KERNEL spectrum::SampledSpectrum directInMedium(const Paths<T, X> & paths, const volume::Coefficients & coefficients, const glm::vec3 & at,
    const glm::vec3 & forward, const PathState & state) {
    const spectrum::SampledSpectrum none(0.0F);
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    const float time = MOYA_PATH(state, ray).time;
    const float held = paths.surfaces.held(time);
    surface::ShadingPoint point;
    point.position = at;
    point.shading = glm::vec3 { 0.0F };
    const light::Picked picked = pickLight(paths, point, held, state);
    if (!(picked.chance > 0.0F)) {
        return none;
    }
    const light::Light & chosen = paths.lights.lights[picked.place];
    const bool alone = light::sampledAlone(chosen);
    if (paths.settings.strategy == Strategy::Bsdf && !alone) {
        return none;
    }
    const light::LightSample drawn = drawLight(paths, picked.place, picked.primitive, at, held, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kLightPoint)));
    if (!drawn.found()) {
        return none;
    }
    const float cosine = glm::dot(forward, drawn.incoming);
    const spectrum::SampledSpectrum value = volume::phaseValue(coefficients, cosine);
    if (value.black()) {
        return none;
    }
    const spectrum::SampledSpectrum emitted = arrivingFrom(paths, picked.place, held, at, drawn, MOYA_PATH(state, wavelengths));
    if (emitted.black()) {
        return none;
    }
    geometry::Ray shadow;
    shadow.origin = at;
    shadow.direction = drawn.incoming;
    shadow.start = 0.0F;
    shadow.end = std::numeric_limits<float>::infinity();
    shadow.time = time;
    shadow.kind = geometry::kShadowRay;
    if (chosen.shape != light::Shape::Distant && chosen.shape != light::Shape::Dome) {
        shadow.direction = drawn.point - at;
        shadow.end = 1.0F - kShadowShort;
    }
    const Passed passed = passing(paths, chosen, shadow, state);
    if (!(passed.share > 0.0F)) {
        return none;
    }
    const float density = picked.chance * drawn.density;
    const bool weighed = paths.settings.strategy == Strategy::Mis && !alone;
    const float weight = weighed ? powerHeuristic(density, volume::phaseDensity(coefficients, cosine) * passed.chance) : 1.0F;
    const spectrum::SampledSpectrum out = value * emitted * passed.through * (passed.share * weight / density);
    gathered(paths, state, litBy(chosen), MOYA_PATH(state, throughput) * out);
    return out;
}

};  // namespace v3d::moya::kernel::integrate
