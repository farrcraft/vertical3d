/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::light {

/**
 * Every light, which instance in the world places each light's shape, and the chance of each
 * being drawn. A light is drawn in proportion to its power: cdf holds the sum of the chances up
 * to and including each light, and is empty when no light has any power. A mesh light's triangle
 * is then drawn in proportion to its area: triangles holds the sums of each mesh light's chances
 * in the same way, from its own first.
 **/
struct Lights {
    Array<Light> lights;

    /** The transformation of each light that moves, at each of its samples. **/
    Array<geometry::Key> keys;

    /** The places of the distant lights with an angle, which a ray that leaves the scene meets. **/
    Array<std::uint32_t> distant;

    /** Each light's place among the lights, by its instance's index in the scene, or none. **/
    Array<std::uint32_t> byId;

    Array<float> cdf;

    Array<float> triangles;

    /**
     * The place of the first mesh light that an instance in the world places, by that instance's
     * index in the scene, or none. The mesh lights an instance places follow each other.
     **/
    Array<std::uint32_t> meshes;

    /**
     * The place, among count chances from a first, of one drawn by a random number, from the sums
     * of the chances up to and including each. None when count is zero.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the first, then how many
    MOYA_KERNEL static std::uint32_t drawnFrom(const Array<float> & sums, std::uint32_t first, std::uint32_t count, float u) {
        if (count == 0U) {
            return geometry::kNoPrimitive;
        }
        std::uint32_t low = 0;
        std::uint32_t high = count - 1U;
        while (low < high) {
            const std::uint32_t middle = low + ((high - low) / 2U);
            if (u < sums[first + middle]) {
                high = middle;
            } else {
                low = middle + 1U;
            }
        }
        return low;
    }

    /** The chance of a place among count chances from a first, from sums drawnFrom() reads. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the first, then how many
    MOYA_KERNEL static float chanceIn(const Array<float> & sums, std::uint32_t first, std::uint32_t count, std::uint32_t place) {
        if (place >= count) {
            return 0.0F;
        }
        return place == 0U ? sums[first] : sums[first + place] - sums[first + place - 1U];
    }

    /** The place among the lights of the light whose shape a hit met, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t at(const geometry::Hit & hit) const {
        if (!hit.found() || hit.depth != 1U) {
            return geometry::kNoPrimitive;
        }
        const std::uint32_t id = hit.instances[0];
        return id < byId.size ? byId[id] : geometry::kNoPrimitive;
    }

    /** The transformation into the world of the light at a place, at a time within the shutter. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the time
    MOYA_KERNEL glm::mat4 placement(std::uint32_t place, float time) const {
        const Light & light = lights[place];
        return light.keyCount > 1U ? geometry::transformAt(&keys[light.keys], light.keyCount, time) : light.transform;
    }

    /** The chance that pick() draws the light at a place. **/
    MOYA_KERNEL float chance(std::uint32_t place) const {
        return chanceIn(cdf, 0U, cdf.size, place);
    }

    /** The place of a light drawn in proportion to its power, or none when no light has any. **/
    MOYA_KERNEL std::uint32_t pick(float u) const {
        return drawnFrom(cdf, 0U, cdf.size, u);
    }

    /** A triangle of the mesh light at a place, drawn in proportion to its area. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the random number
    MOYA_KERNEL std::uint32_t pickTriangle(std::uint32_t place, float u) const {
        const Light & light = lights[place];
        return drawnFrom(triangles, light.triangles, light.count, u);
    }

    /** The chance that pickTriangle() draws a triangle of the mesh light at a place. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the triangle
    MOYA_KERNEL float triangleChance(std::uint32_t place, std::uint32_t primitive) const {
        const Light & light = lights[place];
        return chanceIn(triangles, light.triangles, light.count, primitive);
    }

    /** The place of the mesh light whose geometry a hit's chain places, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t emitting(const geometry::Hit & hit) const {
        if (!hit.found() || hit.depth == 0U || hit.instances[0] >= meshes.size) {
            return geometry::kNoPrimitive;
        }
        for (std::uint32_t place = meshes[hit.instances[0]]; place < lights.size; ++place) {
            const Light & light = lights[place];
            if (light.shape != Shape::Mesh || light.chain[0] != hit.instances[0]) {
                break;
            }
            bool same = light.depth == hit.depth;
            for (std::uint32_t level = 1; same && level < hit.depth; ++level) {
                same = light.chain[level] == hit.instances[level];
            }
            if (same) {
                return place;
            }
        }
        return geometry::kNoPrimitive;
    }

    /**
     * A triangle of the mesh light at a place, in the world at a time within the shutter. The
     * normal is the unit normal of the side it emits from: the side its winding faces, turned as a
     * normal is. False for a light the surfaces do not place.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, the triangle, then the time
    MOYA_KERNEL bool triangleAt(const surface::Surfaces & surfaces, std::uint32_t place, std::uint32_t primitive, float time, glm::vec3 (&corners)[3],
        glm::vec3 & normal) const {  // NOLINT(bugprone-easily-swappable-parameters)
        const Light & light = lights[place];
        glm::mat4 toWorld { 1.0F };
        std::uint32_t placed = 0;
        std::uint32_t material = 0;
        if (!surfaces.chainAt(light.chain, light.depth, time, toWorld, placed, material)) {
            return false;
        }
        glm::vec3 local[3];
        surfaces.corners(placed, primitive, time, local);
        for (std::uint32_t corner = 0; corner < 3U; ++corner) {
            corners[corner] = glm::vec3(toWorld * glm::vec4(local[corner], 1.0F));
        }
        const glm::vec3 turned = glm::transpose(glm::inverse(glm::mat3(toWorld))) * glm::cross(local[1] - local[0], local[2] - local[0]);
        if (!(glm::dot(turned, turned) > 0.0F)) {
            return false;
        }
        normal = glm::normalize(turned);
        return true;
    }

    /**
     * The radiance the light at a place sends back along a ray that met its shape, as its shape
     * stood at a time within the shutter.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the place, then the time
    MOYA_KERNEL spectrum::SampledSpectrum emitted(std::uint32_t place, float time, const geometry::Ray & ray, const geometry::Hit & hit,
        const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) const {
        return radiance(lights[place], placement(place, time), ray.at(hit.distance), -glm::normalize(ray.direction), table, wavelengths);
    }
};

};  // namespace v3d::moya::kernel::light
