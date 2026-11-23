/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/**
 * Perfect specular reflection, in a frame whose z is the shading normal. Light from one direction
 * leaves along one other, its reflection about the normal, on the same side. Its closure's weight
 * is the share of light it reflects.
 **/
struct Mirror {
    /** The direction light comes in from for outgoing: its reflection about the normal. **/
    MOYA_KERNEL static glm::vec3 sample(const glm::vec3 & outgoing) {
        return glm::vec3 { -outgoing.x, -outgoing.y, outgoing.z };
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
