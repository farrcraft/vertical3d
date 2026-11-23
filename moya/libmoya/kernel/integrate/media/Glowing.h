/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/media/Crossings.h>
#include <moya/libmoya/kernel/integrate/media/Media.h>
#include <moya/libmoya/kernel/light/Brick.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Medium.h>

#include <cmath>
#include <cstdint>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::integrate::media {

/*
 * A volume light emits from its bricks. A light sample draws a brick, then a cell of a grid's brick
 * by the cell's chance, and a point uniformly in the cell. A closed surface's one brick is its box,
 * and a point is drawn uniformly in it. The density of the point is by volume in the world, times
 * its distance squared from the point that draws it. That is a density by solid angle and by
 * length along the direction, the measure in which a path tracked through the medium finds it.
 */

/**
 * The count a light sample's draws of a point in a volume light take among a bounce's collisions,
 * apart from the path's own and from a shadow ray's.
 **/
inline constexpr std::uint32_t kGlowDraws = 1U << 30U;

/** The place of the light of a volume, by its place among the volumes, or kNoPrimitive. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL std::uint32_t lightOfVolume(const Paths<T, X> & paths, std::uint32_t volume) {
    return volume < paths.lights.glowing.size ? paths.lights.glowing[volume] : geometry::kNoPrimitive;
}

/** The place of the light of the closed surface of a key, or kNoPrimitive. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL std::uint32_t lightOfEnclosure(const Paths<T, X> & paths, std::uint32_t key) {
    for (std::uint32_t i = 0; i < paths.lights.enclosing.size; ++i) {
        const std::uint32_t place = paths.lights.enclosing[i];
        const light::Light & light = paths.lights.lights[place];
        if (keyOf(light.chain, light.depth) == key) {
            return place;
        }
    }
    return geometry::kNoPrimitive;
}

/** Whether a point of a light's own space lies inside a brick's box. **/
MOYA_KERNEL inline bool holds(const light::Brick & brick, const glm::vec3 & local) {
    return !(local.x < brick.lower.x) && !(local.y < brick.lower.y) && !(local.z < brick.lower.z) && local.x < brick.upper.x && local.y < brick.upper.y &&
        local.z < brick.upper.z;
}

/** The number of the cell of a grid's brick that holds a point of its own space inside it. **/
MOYA_KERNEL inline std::uint32_t cellOf(const light::Brick & brick, const glm::vec3 & local) {
    const glm::vec3 within = glm::clamp(glm::floor(local - brick.lower), glm::vec3(0.0F), glm::vec3(static_cast<float>(light::kBrickSide - 1U)));
    const auto x = static_cast<std::uint32_t>(within.x);
    const auto y = static_cast<std::uint32_t>(within.y);
    const auto z = static_cast<std::uint32_t>(within.z);
    return x + (light::kBrickSide * (y + (light::kBrickSide * z)));
}

/**
 * The chance by the volume of the light's own space with which a point inside a brick is drawn,
 * once the brick is drawn: its cell's chance, or one over a closed surface's box.
 **/
MOYA_KERNEL inline float ownDensity(const light::Lights & lights, const light::Brick & brick, const glm::vec3 & local) {
    if (brick.cells == geometry::kNoPrimitive) {
        const glm::vec3 size = brick.upper - brick.lower;
        const float volume = size.x * size.y * size.z;
        return volume > 0.0F ? 1.0F / volume : 0.0F;
    }
    return light::Lights::chanceIn(lights.cells, brick.cells, light::kBrickCells, cellOf(brick, local));
}

/**
 * The density of a point of the world drawn in a volume light from another point, from its density
 * by the volume of the light's own space. That is divided by the volume the world gives a unit of
 * the space, and multiplied by the distance squared between the points.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the own density, the point, the other
MOYA_KERNEL inline float worldDensity(float own, const glm::mat4 & toWorld, const glm::vec3 & point, const glm::vec3 & from) {
    const float stretch = std::fabs(glm::determinant(glm::mat3(toWorld)));
    const glm::vec3 between = point - from;
    return stretch > 0.0F ? own / stretch * glm::dot(between, between) : 0.0F;
}

/**
 * The brick of the volume light at a place that holds a point of the world, at a time within the
 * shutter, or kNoPrimitive. A grid's bricks are sorted by their lower corners, z first, then y,
 * then x, so the brick whose corner lies below the point's cell is found by halving.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the time
MOYA_KERNEL std::uint32_t brickAt(const Paths<T, X> & paths, std::uint32_t place, const glm::vec3 & point, float time) {
    const light::Light & light = paths.lights.lights[place];
    glm::mat4 toWorld { 1.0F };
    if (light.count == 0U || !paths.lights.ownToWorld(paths.surfaces, place, time, toWorld)) {
        return geometry::kNoPrimitive;
    }
    const glm::vec3 local = glm::vec3(glm::inverse(toWorld) * glm::vec4(point, 1.0F));
    if (light.geometry == geometry::kNoPrimitive) {
        return holds(paths.lights.bricks[light.bricks], local) ? 0U : geometry::kNoPrimitive;
    }
    const float side = static_cast<float>(light::kBrickSide);
    const glm::vec3 corner = glm::floor(local / side) * side;
    const auto before = [&](const glm::vec3 & lower) {
        if (lower.z != corner.z) {
            return lower.z < corner.z;
        }
        if (lower.y != corner.y) {
            return lower.y < corner.y;
        }
        return lower.x < corner.x;
    };
    std::uint32_t low = 0;
    std::uint32_t high = light.count;
    while (low < high) {
        const std::uint32_t middle = low + ((high - low) / 2U);
        if (before(paths.lights.bricks[light.bricks + middle].lower)) {
            low = middle + 1U;
        } else {
            high = middle;
        }
    }
    return low < light.count && paths.lights.bricks[light.bricks + low].lower == corner ? low : geometry::kNoPrimitive;
}

/**
 * The density with which glowingSample() draws a point of the world in a brick of the volume
 * light at a place, from another point, at a time within the shutter. Zero for a point outside
 * the brick.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, the brick, then the points
MOYA_KERNEL float glowingDensity(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t brick, const glm::vec3 & from, const glm::vec3 & point,
    float time) {  // NOLINT(bugprone-easily-swappable-parameters)
    const light::Light & light = paths.lights.lights[place];
    glm::mat4 toWorld { 1.0F };
    if (brick >= light.count || !paths.lights.ownToWorld(paths.surfaces, place, time, toWorld)) {
        return 0.0F;
    }
    const light::Brick & held = paths.lights.bricks[light.bricks + brick];
    const glm::vec3 local = glm::vec3(glm::inverse(toWorld) * glm::vec4(point, 1.0F));
    if (!holds(held, local)) {
        return 0.0F;
    }
    return worldDensity(ownDensity(paths.lights, held, local), toWorld, point, from);
}

/**
 * A point drawn in a brick of the volume light at a place, from a point, at a time within the
 * shutter. A grid's brick draws its cell by u's first number and the point in it by u's second
 * and both of more. A closed surface's box draws its point by u's numbers and more's first.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, the brick, then the time
MOYA_KERNEL light::LightSample glowingSample(const Paths<T, X> & paths, std::uint32_t place, std::uint32_t brick, const glm::vec3 & from, float time,
    const glm::vec2 & u, const glm::vec2 & more) {  // NOLINT(bugprone-easily-swappable-parameters)
    const light::Light & light = paths.lights.lights[place];
    glm::mat4 toWorld { 1.0F };
    if (brick >= light.count || !paths.lights.ownToWorld(paths.surfaces, place, time, toWorld)) {
        return light::LightSample {};
    }
    const light::Brick & held = paths.lights.bricks[light.bricks + brick];
    glm::vec3 local { 0.0F };
    if (held.cells == geometry::kNoPrimitive) {
        local = held.lower + ((held.upper - held.lower) * glm::vec3 { u.x, u.y, more.x });
    } else {
        const std::uint32_t cell = light::Lights::drawnFrom(paths.lights.cells, held.cells, light::kBrickCells, u.x);
        const std::uint32_t x = cell % light::kBrickSide;
        const std::uint32_t y = (cell / light::kBrickSide) % light::kBrickSide;
        const std::uint32_t z = cell / (light::kBrickSide * light::kBrickSide);
        const glm::vec3 corner { static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
        local = held.lower + corner + glm::vec3 { u.y, more.x, more.y };
    }
    light::LightSample out;
    out.point = glm::vec3(toWorld * glm::vec4(local, 1.0F));
    const glm::vec3 between = out.point - from;
    if (!(glm::dot(between, between) > 0.0F)) {
        return light::LightSample {};
    }
    out.incoming = glm::normalize(between);
    out.density = worldDensity(ownDensity(paths.lights, held, local), toWorld, out.point, from);
    return out;
}

/** An emission in ACEScg as a spectrum at the wavelengths a path carries. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL spectrum::SampledSpectrum glowOf(const Paths<T, X> & paths, const glm::vec3 & emission, const spectrum::SampledWavelengths & wavelengths) {
    if (!(emission.x > 0.0F) && !(emission.y > 0.0F) && !(emission.z > 0.0F)) {
        return spectrum::SampledSpectrum(0.0F);
    }
    return spectrum::illuminant(paths.table, emission, wavelengths);
}

/**
 * What the volume light at a place emits at a point of the world, at a time within the shutter: a
 * volume's medium's emission there, or the emission of the medium a closed surface encloses.
 * Whether a closed surface's point lies in its medium is for the shadow ray that reaches it to
 * say.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the time
MOYA_KERNEL spectrum::SampledSpectrum glowingEmission(const Paths<T, X> & paths, std::uint32_t place, const glm::vec3 & point, float time,
    const spectrum::SampledWavelengths & wavelengths) {
    const light::Light & light = paths.lights.lights[place];
    glm::mat4 toWorld { 1.0F };
    std::uint32_t placed = 0;
    std::uint32_t material = surface::kNoMaterial;
    if (!paths.surfaces.placedBy(light.chain, light.depth, time, toWorld, placed, material)) {
        return spectrum::SampledSpectrum(0.0F);
    }
    if (light.geometry == geometry::kNoPrimitive) {
        return material < paths.surfaces.materials.size ? glowOf(paths, paths.surfaces.materials[material].medium.emission, wavelengths)
                                                        : spectrum::SampledSpectrum(0.0F);
    }
    const glm::vec3 local = glm::vec3(glm::inverse(toWorld) * glm::vec4(point, 1.0F));
    const volume::Medium medium = mediumInside(paths.surfaces, paths.textures, paths.volumes, paths.volumes.volumes[light.geometry], local, toWorld, time);
    return glowOf(paths, medium.emission, wavelengths);
}

};  // namespace v3d::moya::kernel::integrate::media
