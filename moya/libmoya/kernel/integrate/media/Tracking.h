/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Dimensions.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/LightSamples.h>
#include <moya/libmoya/kernel/integrate/media/Collisions.h>
#include <moya/libmoya/kernel/integrate/media/Crossings.h>
#include <moya/libmoya/kernel/integrate/media/Glowing.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>
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

/** What a path's tracking through the medium it is in does before the surface it next meets. **/
enum class Tracked : std::uint8_t {
    Reached,
    Scattered,
    Ended
};

/**
 * What the homogeneous medium a path is in emits, in ACEScg, and the place of the light of the
 * closed surface that encloses it, or kNoPrimitive. A path in the air is in a medium that emits
 * nothing.
 **/
struct Glow {
    glm::vec3 emission { 0.0F };
    std::uint32_t light { geometry::kNoPrimitive };
};

/**
 * The share of what a source in a medium emits at a point that a path gathers by colliding there,
 * at a time within the shutter. own is the density with which the path collided there, over the
 * density with which a light sample's shadow ray would have passed the volumes on its way.
 *
 * A source of no light, which no light sample draws, gives all of it, as does a camera ray, a ray
 * after a specular bounce, and any ray while the BSDF alone finds lights. While light samples alone
 * find lights, it gives none. Otherwise it gives the power heuristic's weight of the path's
 * collision against a light sample of the light, drawn from where the path last scattered, of the
 * set of linked lights there. A point in no brick of the light is one no light sample draws.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the light, the point, then the time
MOYA_KERNEL float glowShare(const Paths<T, X> & paths, const PathState & state, std::uint32_t place, const glm::vec3 & point, float time, float own) {
    if (place == geometry::kNoPrimitive || MOYA_PATH(state, bounces) == 0U) {
        return 1.0F;
    }
    const std::uint32_t set = MOYA_PATH(state, linked);
    if (!paths.lights.links.lights(set, place)) {
        return 0.0F;
    }
    if (MOYA_PATH(state, specular) != 0U || paths.settings.strategy == Strategy::Bsdf) {
        return 1.0F;
    }
    if (paths.settings.strategy == Strategy::Light) {
        return 0.0F;
    }
    const std::uint32_t brick = brickAt(paths, place, point, time);
    if (brick == geometry::kNoPrimitive) {
        return 1.0F;
    }
    const glm::vec3 & from = MOYA_PATH(state, from);
    const float density =
        pickChance(paths, set, place, brick, from, MOYA_PATH(state, normal), time) * glowingDensity(paths, place, brick, from, point, time);
    return powerHeuristic(own, density);
}

/**
 * What a path gathers from one source's emission, in ACEScg, at a point it collides at: its share
 * of the emission, times what it carries there. It records the event of the source's light, or of
 * an emitting object where no light draws the source.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the light, the emission, then the point
MOYA_KERNEL void gatherGlow(const Paths<T, X> & paths, PathState & state, std::uint32_t place, const glm::vec3 & emission, const glm::vec3 & point,
    const spectrum::SampledSpectrum & carried, float own) {
    const spectrum::SampledSpectrum glowing = glowOf(paths, emission, MOYA_PATH(state, wavelengths));
    if (glowing.black()) {
        return;
    }
    const float share = glowShare(paths, state, place, point, paths.surfaces.held(MOYA_PATH(state, ray).time), own);
    if (!(share > 0.0F)) {
        return;
    }
    const spectrum::SampledSpectrum lit = carried * glowing * share;
    MOYA_PATH(state, radiance) += lit;
    Event event;
    event.type = EventType::Object;
    gathered(paths, state, place == geometry::kNoPrimitive ? event : litBy(paths.lights.lights[place]), lit);
}

/**
 * What a path gathers at a collision in media from what they emit there, as a tracked path
 * gathers it after pbrt-v4: what it carries times the emission, over the density of the
 * collision. passed is the majorant's transmittance since the last collision, collided that times
 * the majorant at the collision, and lightened what a shadow ray would have passed of the volumes
 * since the last collision. The density with which the path collided grows from its own density
 * by collided, by rescale()'s average, over what lightened grows the lightChances by. Each source
 * is gathered apart: the homogeneous medium the path is in, and each volume crossings holds, whose
 * emission glows holds. Crossings may be none.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void glowAt(const Paths<T, X> & paths, PathState & state, const Glow & around, const Crossings * crossings, const glm::vec3 * glows,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the spectra
    const glm::vec3 & point, const spectrum::SampledSpectrum & passed, const spectrum::SampledSpectrum & collided, const spectrum::SampledSpectrum & lightened) {
    bool any = around.emission.x > 0.0F || around.emission.y > 0.0F || around.emission.z > 0.0F;
    for (std::uint32_t i = 0; crossings != nullptr && i < crossings->count && i < kMostCrossed; ++i) {
        any = any || glows[i].x > 0.0F || glows[i].y > 0.0F || glows[i].z > 0.0F;
    }
    if (!any) {
        return;
    }
    const float grown = (MOYA_PATH(state, chances) * collided).average();
    if (!(grown > 0.0F)) {
        return;
    }
    const float lighter = (MOYA_PATH(state, lightChances) * lightened).average();
    const float own = lighter > 0.0F ? MOYA_PATH(state, density) * grown / lighter : std::numeric_limits<float>::infinity();
    const spectrum::SampledSpectrum carried = MOYA_PATH(state, throughput) * passed / grown;
    gatherGlow(paths, state, around.light, around.emission, point, carried, own);
    for (std::uint32_t i = 0; crossings != nullptr && i < crossings->count && i < kMostCrossed; ++i) {
        gatherGlow(paths, state, lightOfVolume(paths, crossings->entries[i].volume), glows[i], point, carried, own);
    }
}

/** Whether a path may scatter in a medium once more, within its depth limits. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL bool mayScatter(const Paths<T, X> & paths, const PathState & state) {
    return MOYA_PATH(state, bounces) < paths.settings.maxDepth && MOYA_PATH(state, volumeBounces) < paths.settings.volumeDepth;
}

/**
 * Scatters a path at a point in a medium of some coefficients, a distance along its ray, once
 * tracking has weighed it by the collision. It takes a light sample and draws a direction from the
 * medium's phase function by the bounce's draws, as a bounce off a surface does, and counts a
 * volume bounce. A direction the phase function draws records an event in the medium, which the
 * path goes on from. The point is lit by the lights that light an instance of no links. Its cone
 * spreads as a lobe of roughness one spreads it. Russian roulette treats it as any bounce. It
 * gives a denoiser nothing past it.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, the direction, the distance
MOYA_KERNEL Tracked scatterAt(const Paths<T, X> & paths, PathState & state, const volume::Coefficients & coefficients, const glm::vec3 & point,
    const glm::vec3 & forward, float along) {
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    spectrum::SampledSpectrum & throughput = MOYA_PATH(state, throughput);
    MOYA_PATH(state, linked) = 0U;
    MOYA_PATH(state, radiance) += throughput * directInMedium(paths, coefficients, point, forward, state);
    const volume::PhaseSample drawn = volume::phaseSample(coefficients, forward, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kScatter)),
        paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kScatterChoice)));
    if (!drawn.found()) {
        return Tracked::Ended;
    }
    advance(paths, state, scatteredInMedium());
    ++MOYA_PATH(state, volumeBounces);
    throughput *= drawn.value / drawn.density;
    MOYA_PATH(state, density) = drawn.density;
    MOYA_PATH(state, lightChances) = MOYA_PATH(state, chances);
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

/**
 * Tracks a path through the homogeneous medium of some coefficients it is in, along its ray up
 * to a distance in the ray's lengths, which is endless when the ray meets nothing.
 *
 * The distance to a collision is drawn from the medium's extinction at the wavelength trackedOf()
 * names, and a collision scatters with the chance of that wavelength's scattering over its
 * extinction, and is otherwise absorbed. Both numbers come from collisionDraw(), and collisions
 * counts the draws the bounce has made. Each wavelength is weighed by rescale(): by the
 * transmittance to the end for a path that reaches it, and by the scattering times the
 * transmittance to the collision for one that scatters. A medium whose coefficients differ by
 * wavelength then stays unbiased at every wavelength. The density that a light the path meets
 * next is weighed by grows by what rescale() returns. A path that is absorbed ends there, as does
 * one whose scattering a depth limit refuses. One that scatters does as scatterAt() says. At a
 * collision, before any of these, the path gathers what the medium emits, of a glow, as glowAt()
 * says.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the count of draws
MOYA_KERNEL Tracked trackHomogeneous(const Paths<T, X> & paths, PathState & state, const volume::Coefficients & coefficients, const Glow & glow, float end,
    std::uint32_t & collisions) {
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
    const glm::vec3 forward = ray.direction / length;
    const float along = (ray.start * length) + distance;
    const spectrum::SampledSpectrum passed = volume::transmittance(extinction, distance);
    glowAt(paths, state, glow, nullptr, nullptr, ray.origin + (forward * along), passed, passed * extinction, spectrum::SampledSpectrum(1.0F));
    const spectrum::SampledSpectrum scattered = coefficients.scattered();
    if (!(u.y * extinction[tracked] < scattered[tracked])) {
        return Tracked::Ended;
    }
    if (!mayScatter(paths, state)) {
        return Tracked::Ended;
    }
    rescale(throughput, MOYA_PATH(state, chances), scattered * passed);
    if (throughput.black()) {
        return Tracked::Ended;
    }
    return scatterAt(paths, state, coefficients, ray.origin + (forward * along), forward, along);
}

/**
 * Weighs a path that passed a stretch of volumes, or was null at a collision in them. It takes the
 * factor a tracked path would have made it with through rescale(), and the factor a shadow ray
 * would have through lighten(). The density grows by the first's growth over the second's.
 * False when the path carries nothing after it, or no shadow ray could have made it.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the path's factor, then a shadow ray's
MOYA_KERNEL inline bool weighPassing(PathState & state, const spectrum::SampledSpectrum & tracked, const spectrum::SampledSpectrum & shadowed) {
    const float grown = rescale(MOYA_PATH(state, throughput), MOYA_PATH(state, chances), tracked);
    const float lightened = lighten(MOYA_PATH(state, lightChances), shadowed);
    if (MOYA_PATH(state, throughput).black() || !(lightened > 0.0F)) {
        return false;
    }
    MOYA_PATH(state, density) *= grown / lightened;
    return true;
}

/**
 * Tracks a path through the volumes a stretch of its ray crosses, up to a distance in the ray's
 * lengths. It is tracked through the homogeneous medium of some coefficients it is in as well,
 * which are none in the air.
 *
 * The path is tracked by null scattering against a majorant: the homogeneous medium's extinction
 * and the sum of each volume's bound over the cell of it the ray is in. A distance is drawn from
 * the majorant at the wavelength trackedOf() names, and drawn again wherever the majorant
 * changes. At a collision the coefficients of every medium there are joined. The collision
 * scatters, is absorbed or is null with the chance of that wavelength's scattering, absorption or
 * what is left of the majorant, over the majorant. Each number comes from collisionDraw(), and
 * collisions counts the draws the bounce has made.
 *
 * Each wavelength is weighed by rescale(): by the majorant's transmittance since the last
 * collision, times the scattering for a collision that scatters, or times what is left of the
 * majorant for one that is null. A path that reaches the end is weighed by the transmittance
 * alone. A shadow ray tracks only the volumes, so lighten() multiplies the lightChances by the
 * volumes' transmittance, and at a null collision by their majorant too. The density is multiplied
 * by what rescale() returns over what lighten() does. A path that is absorbed ends, as does one
 * whose scattering a depth limit refuses and one that collides more than kMostCollisions times.
 * One that scatters does as scatterAt() says. At every collision, before any of these, the path
 * gathers what the homogeneous medium of a glow and each volume emit there, as glowAt() says.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the count of draws
MOYA_KERNEL Tracked trackCrossed(const Paths<T, X> & paths, PathState & state, const volume::Coefficients & around, const Glow & glow, Crossings & crossings,
    float end, std::uint32_t & collisions) {
    const spectrum::SampledWavelengths & wavelengths = MOYA_PATH(state, wavelengths);
    const spectrum::SampledSpectrum aroundExtinction = around.extinction();
    const geometry::Ray ray = MOYA_PATH(state, ray);
    const float length = glm::length(ray.direction);
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    const int tracked = trackedOf(pixel + paths.sampler.origin, sample);
    spectrum::SampledSpectrum & throughput = MOYA_PATH(state, throughput);
    spectrum::SampledSpectrum passed(1.0F);
    spectrum::SampledSpectrum passedVolumes(1.0F);
    float t = ray.start;
    for (std::uint32_t step = 0; step < kMostCollisions; ++step) {
        spectrum::SampledSpectrum volumes;
        const float next = std::fmin(advanceTo(paths, crossings, t, wavelengths, ray.time, volumes), end);
        const spectrum::SampledSpectrum majorant = aroundExtinction + volumes;
        glm::vec2 u { 0.0F };
        float flight = std::numeric_limits<float>::infinity();
        if (majorant[tracked] > 0.0F) {
            u = collisionDraw(pixel + paths.sampler.origin, sample, bounce, collisions);
            ++collisions;
            flight = volume::freeFlight(majorant[tracked], u.x) / length;
        }
        if (!(t + flight < next)) {
            const float span = (next - t) * length;
            passed *= volume::transmittance(majorant, span);
            passedVolumes *= volume::transmittance(volumes, span);
            t = next;
            if (t < end) {
                continue;
            }
            return weighPassing(state, passed, passedVolumes) ? Tracked::Reached : Tracked::Ended;
        }
        passed *= volume::transmittance(majorant, flight * length);
        passedVolumes *= volume::transmittance(volumes, flight * length);
        t += flight;
        volume::Coefficients here = around;
        glm::vec3 glows[kMostCrossed] {};
        joinCrossed(paths, crossings, t, wavelengths, ray.time, here, glows);
        glowAt(paths, state, glow, &crossings, glows, ray.at(t), passed, passed * majorant, passedVolumes);
        const spectrum::SampledSpectrum scattered = here.scattered();
        const spectrum::SampledSpectrum extinction = here.extinction();
        const float chosen = u.y * majorant[tracked];
        if (chosen < scattered[tracked]) {
            if (!mayScatter(paths, state)) {
                return Tracked::Ended;
            }
            rescale(throughput, MOYA_PATH(state, chances), scattered * passed);
            if (throughput.black()) {
                return Tracked::Ended;
            }
            const glm::vec3 forward = ray.direction / length;
            const float along = t * length;
            return scatterAt(paths, state, here, ray.origin + (forward * along), forward, along);
        }
        if (chosen < extinction[tracked]) {
            return Tracked::Ended;
        }
        if (!weighPassing(state, passed * leftOf(majorant, extinction), passedVolumes * volumes)) {
            return Tracked::Ended;
        }
        passed = spectrum::SampledSpectrum(1.0F);
        passedVolumes = spectrum::SampledSpectrum(1.0F);
    }
    return Tracked::Ended;
}

/**
 * Tracks a path through the homogeneous medium it is in and the volumes its ray crosses, before
 * the surface it next meets. It goes up to a distance in the ray's lengths, which is endless when
 * the ray meets nothing. A ray that crosses a volume is tracked as trackCrossed() says, and one
 * in a homogeneous medium alone as trackHomogeneous() says. A path that crosses no volume reaches
 * the end unchanged when it is in no medium, or in one of no coefficients.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the count of draws
MOYA_KERNEL Tracked track(const Paths<T, X> & paths, PathState & state, float end, std::uint32_t & collisions) {
    const std::uint32_t at = MOYA_PATH(state, media).medium();
    const bool inside = at != surface::kNoMaterial && !volume::empty(paths.surfaces.materials[at].medium);
    volume::Coefficients around {};
    Glow glow;
    if (inside) {
        around = volume::coefficientsOf(paths.table, paths.surfaces.materials[at].medium, MOYA_PATH(state, wavelengths));
        glow.emission = paths.surfaces.materials[at].medium.emission;
        glow.light = lightOfEnclosure(paths, MOYA_PATH(state, media).mediumKey());
    }
    if (paths.volumes.any()) {
        const geometry::Ray & ray = MOYA_PATH(state, ray);
        Crossings crossings = crossed(paths, ray, ray.start, end);
        if (crossings.count != 0U) {
            return trackCrossed(paths, state, around, glow, crossings, end, collisions);
        }
    }
    return inside ? trackHomogeneous(paths, state, around, glow, end, collisions) : Tracked::Reached;
}

};  // namespace v3d::moya::kernel::integrate::media
