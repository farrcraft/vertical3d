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
#include <moya/libmoya/kernel/integrate/media/Collisions.h>
#include <moya/libmoya/kernel/integrate/media/Crossings.h>
#include <moya/libmoya/kernel/integrate/media/Enclosure.h>
#include <moya/libmoya/kernel/integrate/media/Glowing.h>
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
#include <moya/libmoya/kernel/light/spherical/SphericalTriangle.h>
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
 * path tracked through those media would have passed them all, over the density with which the
 * shadow ray tracked the volumes among them. A ray that crosses no medium lets through one at
 * each wavelength, with a chance of one.
 *
 * colliding is the density, by length, with which such a path would then have collided at the
 * ray's end, over the same: its chance times the majorant there. ended is the key of the closed
 * surface whose medium the ray ends in, or media::kNotInside where it ends in none.
 **/
struct Passed {
    float share { 0.0F };
    spectrum::SampledSpectrum through { 1.0F };
    float chance { 1.0F };
    float colliding { 0.0F };
    std::uint32_t ended { media::kNotInside };
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
 * Whether a part of a light is drawn once the light is drawn: a mesh light's triangle, or a volume
 * light's brick.
 **/
MOYA_KERNEL inline bool inParts(const light::Light & light) {
    return light.shape == light::Shape::Mesh || light.shape == light::Shape::Volume;
}

/**
 * The light a light sample draws at a shading point of the path's set of linked lights, at a time
 * within the shutter. The set's light tree draws it, or the power of every light does, with a mesh
 * light's triangle then drawn in proportion to its area, and a volume light's brick in proportion
 * to its emission. Its chance holds the triangle's or the brick's. A light drawn by power that
 * does not light the set draws nothing.
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
    if (out.chance > 0.0F && inParts(paths.lights.lights[out.place])) {
        out.primitive = paths.lights.pickTriangle(out.place, paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kLightTriangle)));
        out.chance *= paths.lights.triangleChance(out.place, out.primitive);
    }
    return out;
}

/**
 * The chance that pickLight() draws the light at a place, and the triangle given of a mesh light
 * or the brick given of a volume light. It is drawn at a shading point of a set of linked lights
 * with a normal, at a time within the shutter.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the set, the place, then the time
MOYA_KERNEL float pickChance(const Paths<T, X> & paths, std::uint32_t set, std::uint32_t place, std::uint32_t primitive, const glm::vec3 & from,
    const glm::vec3 & normal, float time) {  // NOLINT(bugprone-easily-swappable-parameters)
    if (paths.settings.picking == Picking::Tree) {
        return paths.lights.chanceByTree(paths.surfaces, set, place, primitive, from, normal, time);
    }
    const float chance = paths.lights.chance(place);
    return inParts(paths.lights.lights[place]) ? chance * paths.lights.triangleChance(place, primitive) : chance;
}

/**
 * A point drawn on the light at a place from a point, at a time within the shutter, by u. A mesh
 * light's point is drawn on the triangle given, and a volume light's in the brick given, by more
 * as well. A dome light is drawn by its image's compensated way under multiple importance
 * sampling, and by its other way under light samples alone, where nothing else finds what
 * compensation leaves out.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, the triangle, then the time
MOYA_KERNEL light::LightSample drawLight(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t primitive, const glm::vec3 & from, float time,
    const glm::vec2 & u, const glm::vec2 & more) {  // NOLINT(bugprone-easily-swappable-parameters)
    const light::Light & chosen = paths.lights.lights[place];
    if (chosen.shape == light::Shape::Volume) {
        return media::glowingSample(paths, place, primitive, from, time, u, more);
    }
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
    light::spherical::faceTowards(chosen.twoSided, corners, from, normal);
    return light::spherical::triangleSample(corners, normal, from, u);
}

/**
 * The density with which drawLight() draws a point of the light at a place from a point, at a
 * time within the shutter, under multiple importance sampling. A mesh light's point lies on one of
 * its triangles, which a hit names, and its density is on that triangle. A volume light's lies in
 * the brick given.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the triangle
MOYA_KERNEL float lightDensity(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t primitive, const glm::vec3 & from, const glm::vec3 & met,
    float time) {  // NOLINT(bugprone-easily-swappable-parameters)
    const light::Light & light = paths.lights.lights[place];
    if (light.shape == light::Shape::Volume) {
        return media::glowingDensity(paths, place, primitive, from, met, time);
    }
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
    light::spherical::faceTowards(light.twoSided, corners, from, normal);
    return light::spherical::triangleDensity(corners, normal, from, met);
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
    if (chosen.shape == light::Shape::Volume) {
        return media::glowingEmission(paths, place, drawn.point, time, wavelengths);
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
    const volume::Medium * medium = media::mediumOf(paths.surfaces, media);
    if (medium == nullptr || volume::empty(*medium)) {
        return;
    }
    const volume::Coefficients coefficients = volume::coefficientsOf(paths.table, *medium, wavelengths);
    const spectrum::SampledSpectrum passes = volume::transmittance(coefficients.extinction(), distance);
    out.through *= passes;
    tracked *= passes;
}

/**
 * Takes what the volumes along a stretch of a shadow ray let through, from its start up to a
 * parameter, into what passes it, by ratio tracking. False when nothing passes.
 *
 * Distances are drawn against the sum of each volume's bound over the cell of it the ray is in,
 * at the wavelength the path is tracked by, and drawn again wherever that majorant changes. Each
 * number comes from collisionDraw(), counted on from collisions, which starts at
 * kShadowCollisions for each light sample. At each collision every wavelength lets through what
 * is left of the majorant there, and over the last stretch the majorant's transmittance.
 *
 * tracked are the chances the path would have passed so far, and shadowed the chances the shadow
 * ray tracked it by, which start from the path's chances. A collision's factor weighs the light by
 * its value over the shadow ray's density, which lighten() takes into shadowed. tracked take the
 * same factor over the same density, so the chance of passing is their average once shadowed have
 * an average of one again. ending takes the volumes' majorant over the last stretch, or none where
 * the ray crosses no volume.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL bool passVolumes(const Paths<T, X> & paths, const PathState & state, const geometry::Ray & shadow, float to, Passed & out,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the path's chances, then the shadow's
    spectrum::SampledSpectrum & tracked, spectrum::SampledSpectrum & shadowed, std::uint32_t & collisions, spectrum::SampledSpectrum & ending) {
    ending = spectrum::SampledSpectrum(0.0F);
    media::Crossings crossings = media::crossed(paths, shadow, shadow.start, to);
    if (crossings.count == 0U) {
        return true;
    }
    const spectrum::SampledWavelengths & wavelengths = MOYA_PATH(state, wavelengths);
    const glm::uvec2 pixel = MOYA_PATH(state, pixel) + paths.sampler.origin;
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    const int chosen = media::trackedOf(pixel, sample);
    const float length = glm::length(shadow.direction);
    const auto weigh = [&](const spectrum::SampledSpectrum & value, const spectrum::SampledSpectrum & density) {
        const float average = media::lighten(shadowed, density);
        if (!(average > 0.0F)) {
            return false;
        }
        out.through *= value / average;
        tracked *= value / average;
        return !out.through.black();
    };
    spectrum::SampledSpectrum passed(1.0F);
    float t = shadow.start;
    for (std::uint32_t step = 0; step < media::kMostCollisions; ++step) {
        spectrum::SampledSpectrum majorant;
        const float next = std::fmin(media::advanceTo(paths, crossings, t, wavelengths, shadow.time, majorant), to);
        float flight = std::numeric_limits<float>::infinity();
        if (majorant[chosen] > 0.0F) {
            flight = volume::freeFlight(majorant[chosen], media::collisionDraw(pixel, sample, bounce, collisions).x) / length;
            ++collisions;
        }
        if (!(t + flight < next)) {
            passed *= volume::transmittance(majorant, (next - t) * length);
            t = next;
            if (t < to) {
                continue;
            }
            ending = majorant;
            return weigh(passed, passed);
        }
        passed *= volume::transmittance(majorant, flight * length);
        t += flight;
        volume::Coefficients here;
        media::joinCrossed(paths, crossings, t, wavelengths, shadow.time, here);
        if (!weigh(passed * media::leftOf(majorant, here.extinction()), passed * majorant)) {
            return false;
        }
        passed = spectrum::SampledSpectrum(1.0F);
    }
    return false;
}

/**
 * Ends what passes along a shadow ray that reached its end in media. Its chance is the average of
 * the chances a tracked path would have passed it by. Its chance of colliding there is the average
 * of those times the majorant there: the extinction of the medium the ray ends in, and the
 * volumes' majorant over its last stretch. It ends in the closed surface whose medium it is in.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void endIn(const Paths<T, X> & paths, const media::Media & media, const spectrum::SampledWavelengths & wavelengths,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the chances, then the majorant
    const spectrum::SampledSpectrum & tracked, spectrum::SampledSpectrum ending, Passed & out) {
    const volume::Medium * medium = media::mediumOf(paths.surfaces, media);
    if (medium != nullptr && !volume::empty(*medium)) {
        ending += volume::coefficientsOf(paths.table, *medium, wavelengths).extinction();
    }
    out.chance = tracked.average();
    out.colliding = (tracked * ending).average();
    out.ended = media.mediumKey();
}

/**
 * The light that passes along a shadow ray, past everything that casts its shadows. Each surface
 * it meets lets one minus its opacity through, so a solid one lets none through.
 *
 * A ray from a light whose shadow link leaves instances out passes through each of those, from
 * just past where it met it. One that meets more than kMostPassed surfaces, left out or cut out,
 * is taken as blocked.
 *
 * Where any material encloses a medium, the ray carries a copy of media, those of the path it
 * leaves, and passes by the surfaces passedBy() names, entering and leaving their media as a path
 * does. Each medium between lets through its transmittance at each wavelength, by Beer and
 * Lambert's law, and each volume what passVolumes() gives. At the ray's end, the majorant is the
 * extinction of the medium it ends in and the volumes' majorant over its last stretch.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the state, then the media it starts in
MOYA_KERNEL Passed passing(const Paths<T, X> & paths, const light::Light & light, geometry::Ray shadow, const PathState & state, const media::Media & from) {
    Passed out;
    if (paths.surfaces.media == 0U && !paths.volumes.any() && !light.shadowLinked) {
        Passing<X> filter { .surfaces = paths.surfaces, .textures = paths.textures, .ray = shadow };
        out.share = paths.traversal.any(shadow, filter) ? 0.0F : filter.passing;
        return out;
    }
    media::Media media = from;
    spectrum::SampledSpectrum tracked = MOYA_PATH(state, chances);
    spectrum::SampledSpectrum shadowed = MOYA_PATH(state, chances);
    std::uint32_t collisions = media::kShadowCollisions;
    const float length = glm::length(shadow.direction);
    geometry::EveryHit every;
    float through = 1.0F;
    spectrum::SampledSpectrum ending(0.0F);
    for (std::uint32_t passed = 0; passed < kMostPassed; ++passed) {
        const geometry::Hit hit = paths.traversal.nearest(shadow, every);
        const float reached = hit.found() ? hit.distance : shadow.end;
        if (paths.surfaces.media != 0U) {
            attenuate(paths, media, (reached - shadow.start) * length, MOYA_PATH(state, wavelengths), out, tracked);
            if (out.through.black()) {
                return Passed {};
            }
        }
        if (paths.volumes.any() && !passVolumes(paths, state, shadow, reached, out, tracked, shadowed, collisions, ending)) {
            return Passed {};
        }
        if (!hit.found()) {
            out.share = through;
            endIn(paths, media, MOYA_PATH(state, wavelengths), tracked, ending, out);
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
 * The two numbers a light sample draws a point in a volume light by, beside its point's, from the
 * hash collisionDraw() gives at kGlowDraws.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL glm::vec2 glowDraw(const Paths<T, X> & paths, const PathState & state) {
    return media::collisionDraw(MOYA_PATH(state, pixel) + paths.sampler.origin, MOYA_PATH(state, sample), MOYA_PATH(state, bounces), media::kGlowDraws);
}

/**
 * Whether the point a shadow ray reached in a light emits: anywhere in any light but a closed
 * surface's, whose point emits where the ray ends in the surface's own medium.
 **/
MOYA_KERNEL inline bool inGlow(const light::Light & light, const Passed & passed) {
    if (light.shape != light::Shape::Volume || light.geometry != geometry::kNoPrimitive) {
        return true;
    }
    return passed.ended != media::kNotInside && passed.ended == media::keyOf(light.chain, light.depth);
}

/**
 * The media a shadow ray from a shading point starts in: a path's, having entered or left the
 * medium the surface encloses where the ray crosses the surface.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL media::Media shadowMedia(const Paths<T, X> & paths, const geometry::Hit & hit, const surface::ShadingPoint & point, const geometry::Ray & shadow,
    media::Media out) {
    if (paths.surfaces.media == 0U || point.material == surface::kNoMaterial || !paths.surfaces.materials[point.material].encloses()) {
        return out;
    }
    const bool above = glm::dot(shadow.direction, point.geometric) > 0.0F;
    if (above != point.front) {
        out.cross(media::enclosureOf(paths.surfaces, hit, point.material), point.front);
    }
    return out;
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
 * A shadow ray that crosses the surface it leaves enters or leaves the medium the surface
 * encloses, as a path that scatters across it does. Light that crosses media is multiplied by
 * their transmittance, and the closure's density is multiplied by the chance a tracked path
 * would have passed them, as passing() gives both. A tracked path finds a volume light's point by
 * colliding there, so the closure's density is multiplied by passing()'s chance of that instead.
 * The point of a closed surface's light emits only where the shadow ray ends in its medium.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL spectrum::SampledSpectrum direct(const Paths<T, X> & paths, const geometry::Hit & hit, const surface::ShadingPoint & point, const bsdf::Bsdf & scattering,
    const glm::vec3 & outgoing, const PathState & state) {
    const spectrum::SampledSpectrum none(0.0F);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (depthOf(state) >= paths.settings.maxDepth || MOYA_PATH(state, diffuseBounces) >= paths.settings.diffuseDepth) {
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
    const light::LightSample drawn = drawLight(paths, place, picked.primitive, point.position, held,
        paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kLightPoint)), glowDraw(paths, state));
    if (!drawn.found()) {
        return none;
    }
    bsdf::Bsdf::Part parts[bsdf::kMostClosures];
    bool any = false;
    const std::uint64_t evaluating = texture::started(paths.textures);
    for (std::uint32_t i = 0; i < scattering.count && i < bsdf::kMostClosures; ++i) {
        parts[i] = scattering.part(i, outgoing, drawn.incoming);
        any = any || !parts[i].projected.black();
    }
    if (scattering.haired()) {
        texture::finished(paths.textures, texture::Work::Hair, evaluating);
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
    // a shadow ray through media is tracking. One from a point in a medium is timed with the
    // tracking that reached that point, so only a surface's is timed here
    const bool tracks = paths.surfaces.media != 0U || paths.volumes.any();
    const std::uint64_t shadowing = texture::started(paths.textures);
    const Passed passed = paths.surfaces.media != 0U ? passing(paths, chosen, shadow, state, shadowMedia(paths, hit, point, shadow, MOYA_PATH(state, media)))
                                                     : passing(paths, chosen, shadow, state, MOYA_PATH(state, media));
    if (tracks) {
        texture::finished(paths.textures, texture::Work::Tracking, shadowing);
    }
    if (!(passed.share > 0.0F) || !inGlow(chosen, passed)) {
        return none;
    }
    const float density = chance * drawn.density;
    const bool weighed = paths.settings.strategy == Strategy::Mis && !alone;
    const float found = chosen.shape == light::Shape::Volume ? passed.colliding : passed.chance;
    spectrum::SampledSpectrum out(0.0F);
    for (std::uint32_t i = 0; i < scattering.count && i < bsdf::kMostClosures; ++i) {
        const bsdf::Bsdf::Part & part = parts[i];
        if (part.projected.black()) {
            continue;
        }
        const float weight = weighed ? powerHeuristic(density, part.density * found) : 1.0F;
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
 * the point itself. The light is gathered after the event in the medium. A volume light's point is
 * weighed as direct() weighs it.
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
    const light::LightSample drawn =
        drawLight(paths, picked.place, picked.primitive, at, held, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kLightPoint)), glowDraw(paths, state));
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
    const Passed passed = passing(paths, chosen, shadow, state, MOYA_PATH(state, media));
    if (!(passed.share > 0.0F) || !inGlow(chosen, passed)) {
        return none;
    }
    const float density = picked.chance * drawn.density;
    const bool weighed = paths.settings.strategy == Strategy::Mis && !alone;
    const float found = chosen.shape == light::Shape::Volume ? passed.colliding : passed.chance;
    const float weight = weighed ? powerHeuristic(density, volume::phaseDensity(coefficients, cosine) * found) : 1.0F;
    const spectrum::SampledSpectrum out = value * emitted * passed.through * (passed.share * weight / density);
    gathered(paths, state, scatteredInMedium(), litBy(chosen), MOYA_PATH(state, throughput) * out);
    return out;
}

};  // namespace v3d::moya::kernel::integrate
