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
 * Every light with a shape, and which of them each instance in the world places. A hit on a
 * light's shape names its instance, and emitted() gives the radiance the light sends back along
 * the ray.
 **/
struct Lights {
    Array<Light> lights;

    /** Each light's place among the lights, by its instance's index in the scene, or none. **/
    Array<std::uint32_t> byId;

    /** The light whose shape a hit met, or none for a hit on anything else. **/
    MOYA_KERNEL const Light * at(const geometry::Hit & hit) const {
        if (!hit.found() || hit.depth != 1U) {
            return nullptr;
        }
        const std::uint32_t id = hit.instances[0];
        if (id >= byId.size || byId[id] == geometry::kNoPrimitive) {
            return nullptr;
        }
        return &lights[byId[id]];
    }

    /**
     * The radiance a light sends back along a ray that met its shape at a time. Its axis is
     * turned into the world as a normal is, and the cone and a one-sided light's face are measured
     * from it towards where the ray came from.
     **/
    MOYA_KERNEL static spectrum::SampledSpectrum emitted(const Light & light, const surface::Surfaces & surfaces, const geometry::Ray & ray, float time,
        const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
        glm::mat4 toWorld { 1.0F };
        if (!surfaces.placement(light.id, surfaces.held(time), toWorld)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        const glm::vec3 axis = glm::normalize(glm::transpose(glm::inverse(glm::mat3(toWorld))) * light.axis);
        const float cosine = glm::dot(-glm::normalize(ray.direction), axis);
        if (light.oneSided != 0U && !(cosine > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        const float share = coneShare(light, cosine);
        if (!(share > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        return spectrumOf(light, table, wavelengths) * (light.scale * share);
    }
};

};  // namespace v3d::moya::kernel::light
