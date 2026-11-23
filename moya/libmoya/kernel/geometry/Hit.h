/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>
#include <limits>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::geometry {

/** The deepest chain of instances a hit records. An instance nested deeper is not traversed. **/
inline constexpr std::uint32_t kInstanceDepth = 4;

/** The primitive of a hit that found none. **/
inline constexpr std::uint32_t kNoPrimitive = std::numeric_limits<std::uint32_t>::max();

/**
 * Where a ray met a primitive, as small as every device can return it. The stage that shades a
 * hit computes its point, its normals and its differentials from the primitive and these.
 *
 * instances holds the chain of instances that placed the primitive, outermost first, and depth
 * how many of them there are. primitive is the primitive's index within the geometry the
 * innermost instance places. uv is the position on the primitive, which for a triangle is the
 * barycentric weight of its second and third vertices.
 **/
struct Hit {
    float distance { std::numeric_limits<float>::infinity() };
    std::uint32_t primitive { kNoPrimitive };
    glm::vec2 uv { 0.0F };
    float time { 0.0F };
    std::uint32_t instances[kInstanceDepth] {};
    std::uint32_t depth { 0 };

    /** Whether the ray met anything. **/
    MOYA_KERNEL bool found() const {
        return primitive != kNoPrimitive;
    }
};

};  // namespace v3d::moya::kernel::geometry
