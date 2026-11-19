/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/integrate/Inputs.h>
#include <moya/libmoya/kernel/integrate/media/Crossing.h>
#include <moya/libmoya/kernel/program/Fields.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Instance.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Cell.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Field.h>
#include <moya/libmoya/kernel/volume/Grid.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/Volume.h>
#include <moya/libmoya/kernel/volume/Volumes.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/common.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::integrate::media {

/** The most volumes one stretch of a ray is tracked through at once. **/
inline constexpr std::uint32_t kMostCrossed = 8;

/** How far along a ray, in voxels, a point is moved to find the cell the ray enters there. **/
inline constexpr float kCellNudge = 1.0e-3F;

/**
 * The volumes a stretch of a ray crosses, in the order the scene holds them. A stretch that
 * crosses more than kMostCrossed is tracked through the first kMostCrossed alone.
 **/
struct Crossings {
    Crossing entries[kMostCrossed] {};
    std::uint32_t count { 0 };
};

/**
 * The transformation into the world of a volume at a time, through its chain of instances.
 * False for a chain the surfaces do not hold.
 **/
MOYA_KERNEL inline bool placedAt(const surface::Surfaces & surfaces, const volume::Volume & placed, float time, glm::mat4 & toWorld) {
    toWorld = glm::mat4(1.0F);
    for (std::uint32_t level = 0; level < placed.depth && level < geometry::kInstanceDepth; ++level) {
        const std::uint32_t id = placed.chain[level];
        if (id >= surfaces.byId.size || surfaces.byId[id] == geometry::kNoPrimitive) {
            return false;
        }
        const surface::Instance & instance = surfaces.instances[surfaces.byId[id]];
        toWorld = toWorld * (instance.keyCount > 1U ? geometry::transformAt(&surfaces.keys[instance.keys], instance.keyCount, time) : instance.transform);
    }
    return placed.depth != 0U;
}

/**
 * The parameters over which a ray of a volume's own space lies inside the box its fields fill,
 * clipped to a stretch from one parameter to another. Empty when enter is not below exit.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the stretch's start, then its end
MOYA_KERNEL inline bool clipped(const volume::Volume & placed, const glm::vec3 & origin, const glm::vec3 & direction, float from, float to, float & enter,
    float & exit) {
    enter = from;
    exit = to;
    for (glm::length_t axis = 0; axis < 3; ++axis) {
        if (direction[axis] == 0.0F) {
            if (origin[axis] < placed.lower[axis] || origin[axis] > placed.upper[axis]) {
                return false;
            }
            continue;
        }
        const float first = (placed.lower[axis] - origin[axis]) / direction[axis];
        const float second = (placed.upper[axis] - origin[axis]) / direction[axis];
        enter = std::fmax(enter, std::fmin(first, second));
        exit = std::fmin(exit, std::fmax(first, second));
    }
    return enter < exit;
}

/**
 * The volumes a ray crosses between two of its parameters, as they stand at its time, of those
 * that rays of its kind see.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the stretch's start, then its end
MOYA_KERNEL Crossings crossed(const Paths<T, X> & paths, const geometry::Ray & ray, float from, float to) {
    Crossings out;
    const float held = paths.surfaces.held(ray.time);
    for (std::uint32_t index = 0; index < paths.volumes.volumes.size && out.count < kMostCrossed; ++index) {
        const volume::Volume & placed = paths.volumes.volumes[index];
        if ((placed.visibility & ray.kind) == 0U) {
            continue;
        }
        Crossing crossing;
        if (!placedAt(paths.surfaces, placed, held, crossing.toWorld)) {
            continue;
        }
        const glm::mat4 toVolume = glm::inverse(crossing.toWorld);
        crossing.origin = glm::vec3(toVolume * glm::vec4(ray.origin, 1.0F));
        crossing.direction = glm::mat3(toVolume) * ray.direction;
        if (!clipped(placed, crossing.origin, crossing.direction, from, to, crossing.enter, crossing.exit)) {
            continue;
        }
        crossing.volume = index;
        crossing.cellEnd = crossing.enter;
        out.entries[out.count] = crossing;
        ++out.count;
    }
    return out;
}

/** A point of a volume's own space in the index space of the grid of one of its fields. **/
MOYA_KERNEL inline glm::vec3 indexOf(const nanovdb::FloatGrid & grid, const glm::vec3 & point) {
    const nanovdb::Vec3f index = grid.worldToIndexF(nanovdb::Vec3f(point.x, point.y, point.z));
    return glm::vec3 { index[0], index[1], index[2] };
}

/** A direction of a volume's own space in the index space of the grid of one of its fields. **/
MOYA_KERNEL inline glm::vec3 indexDirectionOf(const nanovdb::FloatGrid & grid, const glm::vec3 & direction) {
    const nanovdb::Vec3f index = grid.worldToIndexDirF(nanovdb::Vec3f(direction.x, direction.y, direction.z));
    return glm::vec3 { index[0], index[1], index[2] };
}

/**
 * The medium of a volume where its fields hold values, at a point of the world and a time. A
 * volume whose material has a volume program runs it with those values. Any other scatters its
 * walked field, its density, the same at every wavelength and every way. A coefficient below
 * zero is taken as zero.
 **/
template <geometry::Traversal T, texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the values, then the point
MOYA_KERNEL volume::Medium mediumFrom(const Paths<T, X> & paths, const volume::Volume & placed, const float (&values)[volume::kMostFields], const glm::vec3 & at,
    const glm::mat4 & toWorld, float time) {
    volume::Medium out;
    const surface::Material * material = placed.material < paths.surfaces.materials.size ? &paths.surfaces.materials[placed.material] : nullptr;
    if (material == nullptr || material->mediumProgram.size == 0U) {
        out.lobes[0].scattering = glm::vec3(std::fmax(values[placed.walked], 0.0F));
        return out;
    }
    program::FieldValue fields[volume::kMostFields];
    for (std::uint32_t field = 0; field < placed.fieldCount && field < volume::kMostFields; ++field) {
        fields[field] = program::FieldValue { .name = placed.fields[field].name, .value = values[field] };
    }
    surface::ShadingPoint point;
    point.position = at;
    point.shading = glm::vec3 { 0.0F };
    point.toWorld = toWorld;
    point.time = time;
    out = program::mediumWithFields(paths.surfaces, paths.textures, point, material->mediumProgram, fields, placed.fieldCount);
    out.absorption = glm::max(out.absorption, glm::vec3(0.0F));
    for (volume::Lobe & lobe : out.lobes) {
        lobe.scattering = glm::max(lobe.scattering, glm::vec3(0.0F));
    }
    out.emission = glm::max(out.emission, glm::vec3(0.0F));
    return out;
}

/** The coefficients of the medium of a volume a ray crosses, at a parameter of the ray. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL volume::Coefficients coefficientsAt(const Paths<T, X> & paths, const Crossing & crossing, float t, const spectrum::SampledWavelengths & wavelengths,
    float time) {
    const volume::Volume & placed = paths.volumes.volumes[crossing.volume];
    const glm::vec3 point = crossing.origin + (crossing.direction * t);
    float values[volume::kMostFields] {};
    for (std::uint32_t field = 0; field < placed.fieldCount && field < volume::kMostFields; ++field) {
        const nanovdb::FloatGrid & grid = paths.volumes.gridOf(placed, field);
        values[field] = volume::valueAt(grid, indexOf(grid, point));
    }
    const glm::vec3 world = glm::vec3(crossing.toWorld * glm::vec4(point, 1.0F));
    return volume::coefficientsOf(paths.table, mediumFrom(paths, placed, values, world, crossing.toWorld, time), wavelengths);
}

/**
 * Takes a crossing into the cell of its walked field that a ray enters at a parameter, and bounds
 * the medium's extinction there. A volume that is not bounded is bounded once over all of it.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void enterCell(const Paths<T, X> & paths, Crossing & crossing, float t, const spectrum::SampledWavelengths & wavelengths, float time) {
    const volume::Volume & placed = paths.volumes.volumes[crossing.volume];
    float values[volume::kMostFields] {};
    for (std::uint32_t field = 0; field < placed.fieldCount && field < volume::kMostFields; ++field) {
        values[field] = volume::mostOf(paths.volumes.gridOf(placed, field));
    }
    float end = crossing.exit;
    if (placed.bounded != 0U) {
        const nanovdb::FloatGrid & grid = paths.volumes.gridOf(placed, placed.walked);
        const glm::vec3 origin = indexOf(grid, crossing.origin);
        const glm::vec3 direction = indexDirectionOf(grid, crossing.direction);
        const glm::vec3 probe = origin + (direction * t) + (glm::sign(direction) * kCellNudge);
        const volume::Cell cell = volume::cellAt(grid, glm::ivec3(glm::floor(probe)));
        values[placed.walked] = cell.most;
        end = std::fmin(end, volume::leaving(cell, origin, direction));
        if (!(end > t)) {
            const glm::vec3 across = glm::abs(direction);
            end = std::fmin(crossing.exit, t + (kCellNudge / std::fmax(across.x, std::fmax(across.y, across.z))));
        }
    }
    crossing.cellEnd = end;
    const glm::vec3 point = crossing.origin + (crossing.direction * t);
    const glm::vec3 world = glm::vec3(crossing.toWorld * glm::vec4(point, 1.0F));
    crossing.majorant = volume::coefficientsOf(paths.table, mediumFrom(paths, placed, values, world, crossing.toWorld, time), wavelengths).extinction();
}

/**
 * Brings every crossing up to a parameter of the ray. A crossing the ray is inside there takes
 * the cell it is in, once it has left the one before. majorant takes the sum of the majorants of
 * the crossings the ray is inside. Returns the parameter at which the next of them changes: where
 * the ray enters a volume, or leaves a cell.
 **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL float advanceTo(const Paths<T, X> & paths, Crossings & crossings, float t, const spectrum::SampledWavelengths & wavelengths, float time,
    spectrum::SampledSpectrum & majorant) {
    majorant = spectrum::SampledSpectrum(0.0F);
    float next = std::numeric_limits<float>::infinity();
    for (std::uint32_t i = 0; i < crossings.count && i < kMostCrossed; ++i) {
        Crossing & crossing = crossings.entries[i];
        if (!(t < crossing.exit)) {
            continue;
        }
        if (t < crossing.enter) {
            next = std::fmin(next, crossing.enter);
            continue;
        }
        if (!(t < crossing.cellEnd)) {
            enterCell(paths, crossing, t, wavelengths, time);
        }
        majorant += crossing.majorant;
        next = std::fmin(next, crossing.cellEnd);
    }
    return next;
}

/** Joins into coefficients those of each volume a ray is inside at a parameter. **/
template <geometry::Traversal T, texture::Texels X>
MOYA_KERNEL void joinCrossed(const Paths<T, X> & paths, const Crossings & crossings, float t, const spectrum::SampledWavelengths & wavelengths, float time,
    volume::Coefficients & into) {
    for (std::uint32_t i = 0; i < crossings.count && i < kMostCrossed; ++i) {
        const Crossing & crossing = crossings.entries[i];
        if (!(t < crossing.enter) && t < crossing.exit) {
            volume::join(into, coefficientsAt(paths, crossing, t, wavelengths, time));
        }
    }
}

};  // namespace v3d::moya::kernel::integrate::media
