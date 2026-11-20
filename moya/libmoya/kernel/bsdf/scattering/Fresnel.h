/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/**
 * Fresnel's equations for an interface between two dielectrics. eta is the index of refraction of
 * the side light passes into over the index of the side it comes from.
 **/
struct Fresnel {
    /**
     * The share of unpolarised light reflected at a cosine to the normal, on the side light comes
     * from. Beyond the critical angle, where nothing is transmitted, it is one.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the ratio of indices
    MOYA_KERNEL static float reflected(float cosine, float eta) {
        const float incident = std::fmin(std::fmax(cosine, 0.0F), 1.0F);
        const float sineSquared = (1.0F - (incident * incident)) / (eta * eta);
        if (!(sineSquared < 1.0F)) {
            return 1.0F;
        }
        const float transmitted = std::sqrt(1.0F - sineSquared);
        const float parallel = ((eta * incident) - transmitted) / ((eta * incident) + transmitted);
        const float perpendicular = (incident - (eta * transmitted)) / (incident + (eta * transmitted));
        return 0.5F * ((parallel * parallel) + (perpendicular * perpendicular));
    }

    /**
     * The share a thin slab of the dielectric reflects, both its faces counted: light that enters
     * reflects back and forth inside, and leaves through either face each time. The rest passes
     * through, since the slab absorbs nothing.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the ratio of indices
    MOYA_KERNEL static float slab(float cosine, float eta) {
        const float once = reflected(cosine, eta);
        if (!(once < 1.0F)) {
            return 1.0F;
        }
        const float through = 1.0F - once;
        return once + (through * through * once / (1.0F - (once * once)));
    }

    /**
     * The direction light leaves along after passing through an interface whose normal is normal,
     * for light arriving from direction, which points away from the interface on normal's side.
     * False beyond the critical angle.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the direction, then the normal
    MOYA_KERNEL static bool refract(const glm::vec3 & direction, const glm::vec3 & normal, float eta, glm::vec3 & out) {
        const float incident = glm::dot(direction, normal);
        const float sineSquared = std::fmax(0.0F, 1.0F - (incident * incident)) / (eta * eta);
        if (!(sineSquared < 1.0F)) {
            return false;
        }
        const float transmitted = std::sqrt(1.0F - sineSquared);
        out = glm::normalize((-direction / eta) + (((incident / eta) - transmitted) * normal));
        return true;
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
