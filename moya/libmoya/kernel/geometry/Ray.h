/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <limits>

#include <glm/vec3.hpp>

/**
 * Rays, what they hit, and the tests that find it. Every primitive but the triangle is intersected
 * here on every device. A device's traversal finds which primitives a ray reaches.
 **/
namespace v3d::moya::kernel::geometry {

/**
 * A ray: the points origin + t * direction for t in [start, end), at one time over the shutter.
 * The direction need not be unit length, and a distance along the ray is measured in its
 * lengths.
 **/
struct Ray {
    glm::vec3 origin { 0.0F };
    glm::vec3 direction { 0.0F, 0.0F, -1.0F };
    float start { 0.0F };
    float end { std::numeric_limits<float>::infinity() };
    float time { 0.0F };

    /** The point at t along the ray. **/
    MOYA_KERNEL glm::vec3 at(float t) const {
        return origin + (direction * t);
    }
};

};  // namespace v3d::moya::kernel::geometry
