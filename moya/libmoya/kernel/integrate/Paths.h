/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/AtSurface.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/film/FilterSample.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Covered.h>
#include <moya/libmoya/kernel/integrate/Dimensions.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/Keeping.h>
#include <moya/libmoya/kernel/integrate/LightSamples.h>
#include <moya/libmoya/kernel/integrate/media/Media.h>
#include <moya/libmoya/kernel/integrate/media/Tracking.h>
#include <moya/libmoya/kernel/integrate/PathState.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/light/Dome.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Footprint.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/texture/Texels.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/**
 * The generate stage: a camera sample's state. Its place in the pixel is drawn from the pixel
 * filter, with the filter's weight, and its point on the lens, its time and its wavelengths are
 * drawn after it. It carries a throughput of one and chances of one, and has gathered no light,
 * and its cone is the camera's for its ray. It starts inside the closed surfaces around() finds,
 * when any material encloses a medium. Its first event is the camera.
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
    MOYA_PATH(state, chances) = spectrum::SampledSpectrum(1.0F);
    MOYA_PATH(state, lightChances) = spectrum::SampledSpectrum(1.0F);
    MOYA_PATH(state, radiance) = spectrum::SampledSpectrum(0.0F);
    MOYA_PATH(state, expression) = 0U;
    MOYA_PATH(state, media) = paths.surfaces.media != 0U ? around(paths, MOYA_PATH(state, ray)) : media::Media {};
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
 *
 * A path walking the subsurface of the surface it met, and reflected back into it, makes an event
 * of its walk: it counts against the walk's limit alone, and takes no Russian roulette. One that
 * leaves the surface makes a transmission, as any bounce does. walks says whether the sample was
 * drawn from a closure that enters the medium a subsurface gives.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - whether walking, then whether it walks on
MOYA_KERNEL bool scatter(const Paths<T, X> & paths, const surface::ShadingPoint & point, const bsdf::Bsdf & scattering, const glm::vec3 & outgoing,
    PathState & state, bool walking, bool & walks) {
    const std::uint32_t bounce = MOYA_PATH(state, bounces);
    if (!walking && depthOf(state) >= paths.settings.maxDepth) {
        return false;
    }
    const glm::uvec2 pixel = MOYA_PATH(state, pixel);
    const std::uint32_t sample = MOYA_PATH(state, sample);
    const bsdf::Sample drawn = scattering.sample(outgoing, paths.sampler.get2D(pixel, sample, bounceDimension(bounce, kScatter)),
        paths.sampler.get1D(pixel, sample, bounceDimension(bounce, kScatterChoice)));
    if (!drawn.found()) {
        return false;
    }
    const bool inWalk = walking && drawn.lobe != bsdf::Lobe::Transmission;
    if (inWalk) {
        if (MOYA_PATH(state, subsurfaceBounces) >= paths.settings.subsurfaceDepth) {
            return false;
        }
        ++MOYA_PATH(state, subsurfaceBounces);
    } else if (depthOf(state) >= paths.settings.maxDepth || !counted(paths.settings, drawn.lobe, state)) {
        return false;
    }
    walks = scattering.closures[drawn.closure].walks;
    advance(paths, state, scatteredBy(drawn));
    MOYA_PATH(state, throughput) *= drawn.projected / drawn.density;
    MOYA_PATH(state, density) = drawn.density;
    MOYA_PATH(state, lightChances) = MOYA_PATH(state, chances);
    MOYA_PATH(state, specular) = drawn.specular ? 1U : 0U;
    MOYA_PATH(state, from) = point.position;
    MOYA_PATH(state, normal) = point.shading;
    const bool crossed = drawn.lobe == bsdf::Lobe::Transmission;
    MOYA_PATH(state, cone) =
        surface::bounced(MOYA_PATH(state, cone), point.footprint.width(), crossed ? 0.0F : paths.surfaces.curvature(point), drawn.roughness, crossed);
    MOYA_PATH(state, bounces) = bounce + 1U;
    if (!inWalk && depthOf(state) >= paths.settings.rouletteDepth) {
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
 * Whether a path passes by the surface a hit met, as no bounce, as passesBy() says. It goes on
 * from just past the hit.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL bool passedBy(const Paths<T, X> & paths, PathState & state, const geometry::Hit & hit, const surface::ShadingPoint & point) {
    if (!passesBy(paths, MOYA_PATH(state, media), hit, point)) {
        return false;
    }
    geometry::Ray & ray = MOYA_PATH(state, ray);
    ray.start = std::fmax(hit.distance * (1.0F + kShadowShort), hit.distance + std::numeric_limits<float>::min());
    return true;
}

/**
 * Enters or leaves the medium a surface encloses, when the ray a path leaves a shading point by
 * crosses the surface. A path that enters by a closure that walks walks the subsurface's medium.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void crossing(const Paths<T, X> & paths, PathState & state, const geometry::Hit & hit, const surface::ShadingPoint & point, bool walks) {
    if (point.material == surface::kNoMaterial || !paths.surfaces.materials[point.material].encloses()) {
        return;
    }
    const bool above = glm::dot(MOYA_PATH(state, ray).direction, point.geometric) > 0.0F;
    if (above != point.front) {
        MOYA_PATH(state, media).cross(media::enclosureOf(paths.surfaces, hit, point.material, walks), point.front);
    }
}

/** Marks a camera ray's sample as covered once it meets anything. Later rays leave it. **/
MOYA_KERNEL inline void covers(PathState & state) {
    if (MOYA_PATH(state, bounces) == 0U) {
        MOYA_PATH(state, coverage) = 1.0F;
    }
}

/**
 * What a path gathers from the light whose shape its ray met, which ends it: its share of the
 * radiance the light sends back along the ray, times its throughput. The ray's first hit and what
 * it covers are those of a light, which no surface's channels read.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void meetLight(const Paths<T, X> & paths, PathState & state, const geometry::Hit & hit, std::uint32_t met) {
    const geometry::Ray & ray = MOYA_PATH(state, ray);
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
}

/**
 * What a path gathers at a shading point from each mesh light that makes its geometry emit: its
 * share of what the light emits there, a mesh light's and its material's both, times its
 * throughput. A light seen from behind its triangles emits only when it is two-sided.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void glowAt(const Paths<T, X> & paths, PathState & state, const geometry::Hit & hit, const surface::ShadingPoint & point) {
    for (std::uint32_t glowing = paths.lights.emitting(hit); glowing != geometry::kNoPrimitive; glowing = paths.lights.emitting(hit, glowing)) {
        const light::Light & shining = paths.lights.lights[glowing];
        const float share = point.front || shining.twoSided ? metShare(paths, glowing, hit.primitive, paths.surfaces.held(hit.time), point.position, state) : 0.0F;
        if (share > 0.0F) {
            const spectrum::SampledSpectrum lit = MOYA_PATH(state, throughput) * light::glow(shining, paths.table, MOYA_PATH(state, wavelengths)) * share;
            MOYA_PATH(state, radiance) += lit;
            gathered(paths, state, litBy(shining), lit);
        }
    }
}

/**
 * A path's bounce at the shading point its ray met: the footprint its cone makes there, the light
 * of the mesh lights it meets, a light sample and the scatter stage. False when the path ends
 * there. A path that crosses the surface as it scatters enters or leaves the medium it encloses,
 * and one that leaves through a lobe that is not singular gives a denoiser nothing past it. A path
 * that walks the subsurface of the surface it met meets it from inside, as the subsurface's
 * dielectric alone.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL bool bounce(const Paths<T, X> & paths, PathState & state, const geometry::Hit & hit, surface::ShadingPoint & point) {
    const geometry::Ray & ray = MOYA_PATH(state, ray);
    // a hit's distance is in the ray's lengths, and the cone's in world units
    const float length = glm::length(ray.direction);
    point.footprint = surface::footprintOf(MOYA_PATH(state, cone).widthAt(hit.distance * length), ray.direction / length, point.shading);
    glowAt(paths, state, hit, point);
    MOYA_PATH(state, linked) = paths.lights.links.setOf(hit);
    const glm::vec3 outgoing = -glm::normalize(ray.direction);
    const std::uint32_t key = media::keyOf(hit);
    const float outside = MOYA_PATH(state, media).outside(key);
    const bool walking = MOYA_PATH(state, media).walks(key);
    const bsdf::Bsdf scattering =
        bsdf::atSurface(paths.surfaces, paths.textures, point, paths.table, paths.albedos, MOYA_PATH(state, wavelengths), outgoing, outside, walking);
    features(paths, state, scattering, point, hit.distance * length);
    MOYA_PATH(state, radiance) += MOYA_PATH(state, throughput) * direct(paths, hit, point, scattering, outgoing, state);
    bool walks = false;
    if (!scatter(paths, point, scattering, outgoing, state, walking, walks)) {
        return false;
    }
    crossing(paths, state, hit, point, walks);
    if (MOYA_PATH(state, specular) == 0U) {
        MOYA_PATH(state, featured) = 1U;
    }
    return true;
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
 *
 * A path passes by the surfaces passedBy() names, at most kMostPassed of them, and enters or
 * leaves their media. A dielectric bends light by its own index over that of the medium beyond
 * it, which is the medium the path is in leaving the surface out. A path that crosses a surface
 * as it scatters enters or leaves the medium it encloses. Along each ray, before the surface it
 * meets, track() tracks the path through the medium it is in, where it may scatter or end.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void trace(const Paths<T, X> & paths, PathState & state) {
    std::uint32_t passes = 0;
    std::uint32_t collisions = 0;
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
        const bool tracks = paths.surfaces.media != 0U || paths.volumes.any();
        const media::Tracked tracked = tracks ? media::track(paths, state, hit.found() ? hit.distance : ray.end, collisions) : media::Tracked::Reached;
        switch (tracked) {
            case media::Tracked::Ended:
                return;
            case media::Tracked::Scattered:
                continue;
            case media::Tracked::Reached:
                break;
        }
        if (!hit.found()) {
            firstHit(paths, state, hit, ray, surface::ShadingPoint {});
            cover(paths, state, hit, surface::kNoMaterial, std::numeric_limits<float>::infinity());
            MOYA_PATH(state, radiance) += MOYA_PATH(state, throughput) * leaving(paths, ray, state);
            return;
        }
        const std::uint32_t met = paths.lights.at(hit);
        surface::ShadingPoint point;
        if (met == geometry::kNoPrimitive) {
            point = paths.surfaces.at(hit, ray);
            if (passedBy(paths, state, hit, point)) {
                ++passes;
                if (passes >= kMostPassed) {
                    return;
                }
                continue;
            }
        }
        covers(state);
        if (met != geometry::kNoPrimitive) {
            meetLight(paths, state, hit, met);
            return;
        }
        firstHit(paths, state, hit, ray, point);
        cover(paths, state, hit, point.material, hit.distance);
        if (!point.found() || !bounce(paths, state, hit, point)) {
            return;
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
