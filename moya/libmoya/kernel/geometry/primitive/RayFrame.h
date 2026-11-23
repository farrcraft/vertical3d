/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::geometry::primitive {

/**
 * A ray's own space: its origin at zero, and its direction along +z. The frame is orthonormal and
 * right-handed, so lengths are kept, and a point at the ray's distance t lies at z = t * length.
 **/
struct RayFrame {
    glm::vec3 origin { 0.0F };
    glm::vec3 x { 1.0F, 0.0F, 0.0F };
    glm::vec3 y { 0.0F, 1.0F, 0.0F };
    glm::vec3 z { 0.0F, 0.0F, 1.0F };

    /** The length of the ray's direction. Zero for a direction that is not finite and positive. **/
    float length { 0.0F };

    /** The frame of a ray, by Duff and others' basis, which has no branch where z nears -1. **/
    MOYA_KERNEL static RayFrame of(const Ray & ray) {
        RayFrame out;
        const float length = glm::length(ray.direction);
        if (!(length > 0.0F) || !std::isfinite(length)) {
            return out;
        }
        out.origin = ray.origin;
        out.length = length;
        out.z = ray.direction / length;
        const float sign = std::copysign(1.0F, out.z.z);
        const float scale = -1.0F / (sign + out.z.z);
        const float skew = out.z.x * out.z.y * scale;
        out.x = glm::vec3 { 1.0F + (sign * out.z.x * out.z.x * scale), sign * skew, -sign * out.z.x };
        out.y = glm::vec3 { skew, sign + (out.z.y * out.z.y * scale), -out.z.y };
        return out;
    }

    /** A point moved into the frame, with its fourth component kept. **/
    MOYA_KERNEL glm::vec4 into(const glm::vec4 & point) const {
        const glm::vec3 offset = glm::vec3(point) - origin;
        return glm::vec4 { glm::dot(offset, x), glm::dot(offset, y), glm::dot(offset, z), point.w };
    }
};

};  // namespace v3d::moya::kernel::geometry::primitive
