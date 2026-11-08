/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * The start of a ray that leaves a surface at a point, moved off it along the geometric normal so
 * that the ray does not meet the surface again where it starts. The normal points to the side the
 * ray leaves by, and need not be unit length.
 *
 * Each coordinate moves by a count of representable floats in proportion to the normal, so the
 * move grows with the point's distance from the origin as the rounding of the hit does. Near the
 * origin, where floats are densest, it moves by a small fixed fraction of the normal instead. This
 * is Wächter and Binder's method.
 **/
MOYA_KERNEL inline glm::vec3 offsetStart(const glm::vec3 & point, const glm::vec3 & normal) {
    // the coordinate below which a fixed move replaces the move in floats
    constexpr float kOrigin = 1.0F / 32.0F;
    // the fixed move near the origin, as a fraction of the normal
    constexpr float kFloatScale = 1.0F / 65536.0F;
    // the move in representable floats, per unit of the normal
    constexpr float kIntScale = 256.0F;

    glm::vec3 out { 0.0F };
    for (int axis = 0; axis < 3; ++axis) {
        const auto ulps = static_cast<std::int32_t>(kIntScale * normal[axis]);
        const std::int32_t bits = glm::floatBitsToInt(point[axis]);
        const float moved = glm::intBitsToFloat(point[axis] < 0.0F ? bits - ulps : bits + ulps);
        out[axis] = std::fabs(point[axis]) < kOrigin ? point[axis] + (kFloatScale * normal[axis]) : moved;
    }
    return out;
}

};  // namespace v3d::moya::kernel::geometry
