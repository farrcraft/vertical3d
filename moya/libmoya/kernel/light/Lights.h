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

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/**
 * Every light, which instance in the world places each light's shape, and the chance of each
 * being drawn. A light is drawn in proportion to its power: cdf holds the sum of the chances up
 * to and including each light, and is empty when no light has any power.
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
        if (place >= cdf.size) {
            return 0.0F;
        }
        return place == 0U ? cdf[0] : cdf[place] - cdf[place - 1U];
    }

    /** The place of a light drawn in proportion to its power, or none when no light has any. **/
    MOYA_KERNEL std::uint32_t pick(float u) const {
        if (cdf.size == 0U) {
            return geometry::kNoPrimitive;
        }
        std::uint32_t low = 0;
        std::uint32_t high = cdf.size - 1U;
        while (low < high) {
            const std::uint32_t middle = low + ((high - low) / 2U);
            if (u < cdf[middle]) {
                high = middle;
            } else {
                low = middle + 1U;
            }
        }
        return low;
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
