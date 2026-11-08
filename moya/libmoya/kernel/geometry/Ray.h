/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>
#include <limits>

#include <glm/vec3.hpp>

/**
 * Rays, what they hit, and the tests that find it. Every primitive but the triangle is intersected
 * here on every device. A device's traversal finds which primitives a ray reaches.
 **/
namespace v3d::moya::kernel::geometry {

/** The kinds of ray. Each is one bit of what an instance is visible to. **/
inline constexpr std::uint32_t kCameraRay = 1U;
inline constexpr std::uint32_t kShadowRay = 2U;
inline constexpr std::uint32_t kIndirectRay = 4U;
inline constexpr std::uint32_t kEveryRay = kCameraRay | kShadowRay | kIndirectRay;

/**
 * A ray: the points origin + t * direction for t in [start, end), at one time over the shutter.
 * The direction need not be unit length, and a distance along the ray is measured in its
 * lengths. Its kind is one of the kinds above, and an instance hidden from that kind is not met.
 **/
struct Ray {
    glm::vec3 origin { 0.0F };
    glm::vec3 direction { 0.0F, 0.0F, -1.0F };
    float start { 0.0F };
    float end { std::numeric_limits<float>::infinity() };
    float time { 0.0F };
    std::uint32_t kind { kCameraRay };

    /** The point at t along the ray. **/
    MOYA_KERNEL glm::vec3 at(float t) const {
        return origin + (direction * t);
    }
};

};  // namespace v3d::moya::kernel::geometry
