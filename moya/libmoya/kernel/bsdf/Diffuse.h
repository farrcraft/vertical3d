/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/** One over pi. **/
inline constexpr float kInversePi = 0.318309886F;

/**
 * Lambert's diffuse reflection, in a frame whose z is the shading normal. It reflects into the
 * side of the surface light leaves towards, on either side, with the same value in every
 * direction. Its closure's weight is its reflectance.
 **/
struct Diffuse {
    /** The value for light from incoming leaving along outgoing, for a reflectance of one. **/
    MOYA_KERNEL static float evaluate(const glm::vec3 & outgoing, const glm::vec3 & incoming) {
        return outgoing.z * incoming.z > 0.0F ? kInversePi : 0.0F;
    }

    /** The density sample() draws incoming with: its cosine over pi, on outgoing's side. **/
    MOYA_KERNEL static float density(const glm::vec3 & outgoing, const glm::vec3 & incoming) {
        return outgoing.z * incoming.z > 0.0F ? std::fabs(incoming.z) * kInversePi : 0.0F;
    }

    /**
     * A direction light comes in from, on outgoing's side, drawn by its cosine from two random
     * numbers. The unit square is mapped onto the disk by Shirley and Chiu's concentric mapping,
     * and lifted onto the hemisphere.
     **/
    MOYA_KERNEL static glm::vec3 sample(const glm::vec3 & outgoing, const glm::vec2 & u) {
        const glm::vec2 disk = concentric(u);
        const float lift = std::sqrt(std::fmax(0.0F, 1.0F - (disk.x * disk.x) - (disk.y * disk.y)));
        return glm::vec3 { disk.x, disk.y, outgoing.z < 0.0F ? -lift : lift };
    }

    /** A point of the unit disk from one of the unit square, keeping areas in proportion. **/
    MOYA_KERNEL static glm::vec2 concentric(const glm::vec2 & u) {
        constexpr float kQuarterPi = 0.785398163F;
        const glm::vec2 centred = (u * 2.0F) - 1.0F;
        if (centred.x == 0.0F && centred.y == 0.0F) {
            return glm::vec2 { 0.0F };
        }
        float radius = 0.0F;
        float angle = 0.0F;
        if (std::fabs(centred.x) > std::fabs(centred.y)) {
            radius = centred.x;
            angle = kQuarterPi * (centred.y / centred.x);
        } else {
            radius = centred.y;
            angle = (2.0F * kQuarterPi) - (kQuarterPi * (centred.x / centred.y));
        }
        return glm::vec2 { radius * std::cos(angle), radius * std::sin(angle) };
    }
};

};  // namespace v3d::moya::kernel::bsdf
