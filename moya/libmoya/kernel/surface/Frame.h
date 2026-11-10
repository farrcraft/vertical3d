/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * Three directions at right angles, of unit length, around a normal. A BSDF works in the frame,
 * in which the normal is z.
 **/
struct Frame {
    glm::vec3 tangent { 1.0F, 0.0F, 0.0F };
    glm::vec3 bitangent { 0.0F, 1.0F, 0.0F };
    glm::vec3 normal { 0.0F, 0.0F, 1.0F };

    /**
     * The frame around a normal of unit length, by Duff and others' construction, which has no
     * branch but the normal's sign and no division by a small number.
     **/
    MOYA_KERNEL static Frame around(const glm::vec3 & normal) {
        const float sign = std::copysign(1.0F, normal.z);
        const float inverse = -1.0F / (sign + normal.z);
        const float skew = normal.x * normal.y * inverse;
        Frame out;
        out.tangent = glm::vec3 { 1.0F + (sign * normal.x * normal.x * inverse), sign * skew, -sign * normal.x };
        out.bitangent = glm::vec3 { skew, sign + (normal.y * normal.y * inverse), -normal.y };
        out.normal = normal;
        return out;
    }

    /** A direction in world space, in the frame. **/
    MOYA_KERNEL glm::vec3 toLocal(const glm::vec3 & world) const {
        return glm::vec3 { glm::dot(world, tangent), glm::dot(world, bitangent), glm::dot(world, normal) };
    }

    /** A direction in the frame, in world space. **/
    MOYA_KERNEL glm::vec3 toWorld(const glm::vec3 & local) const {
        return (tangent * local.x) + (bitangent * local.y) + (normal * local.z);
    }
};

};  // namespace v3d::moya::kernel::surface
