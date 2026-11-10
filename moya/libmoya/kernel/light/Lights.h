/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
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

namespace v3d::moya::kernel::light {

/**
 * Every light with a shape, which instance in the world places each, and the chance of each being
 * drawn. A light is drawn in proportion to its power: cdf holds the sum of the chances up to and
 * including each light, and is empty when no light has any power.
 **/
struct Lights {
    Array<Light> lights;

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
     * stood at the hit's time.
     **/
    MOYA_KERNEL spectrum::SampledSpectrum emitted(std::uint32_t place, const surface::Surfaces & surfaces, const geometry::Ray & ray,
        const geometry::Hit & hit, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) const {
        const Light & light = lights[place];
        glm::mat4 toWorld { 1.0F };
        if (!surfaces.placement(light.id, surfaces.held(hit.time), toWorld)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        return radiance(light, toWorld, ray.at(hit.distance), -glm::normalize(ray.direction), table, wavelengths);
    }
};

};  // namespace v3d::moya::kernel::light
