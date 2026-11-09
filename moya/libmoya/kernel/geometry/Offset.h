/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * The start of a ray that leaves a surface at a point, moved off it along the geometric normal so
 * that the ray does not meet the surface again where it starts. The normal points to the side the
 * ray leaves by. It need not be unit length, as only its direction counts. A normal of zero, or
 * one with a component that is infinite or not a number, leaves the point where it is.
 *
 * Each coordinate moves by a count of representable floats in proportion to the unit normal, so
 * the move grows with the point's distance from the origin as the rounding of the hit does. Near
 * the origin, where floats are densest, it moves by a small fixed fraction of the unit normal
 * instead. This is Wächter and Binder's method.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the normal it moves along
MOYA_KERNEL inline glm::vec3 offsetStart(const glm::vec3 & point, const glm::vec3 & normal) {
    // the coordinate below which a fixed move replaces the move in floats
    constexpr float kOrigin = 1.0F / 32.0F;
    // the fixed move near the origin, as a fraction of the unit normal
    constexpr float kFloatScale = 1.0F / 65536.0F;
    // the move in representable floats, per unit of the unit normal
    constexpr float kIntScale = 256.0F;

    // scaled by its largest component first, so a huge normal does not overflow its length
    const float largest = std::fmax(std::fabs(normal.x), std::fmax(std::fabs(normal.y), std::fabs(normal.z)));
    if (!(largest > 0.0F) || !std::isfinite(largest) || std::isnan(normal.x + normal.y + normal.z)) {
        return point;
    }
    const glm::vec3 unit = glm::normalize(normal / largest);

    glm::vec3 out { 0.0F };
    for (int axis = 0; axis < 3; ++axis) {
        if (std::fabs(point[axis]) < kOrigin) {
            out[axis] = point[axis] + (kFloatScale * unit[axis]);
            continue;
        }
        // at most kIntScale floats either way, added to the bits modulo 2^32, so a point that is
        // infinite or not a number gives some float rather than overflowing
        const auto ulps = static_cast<std::int32_t>(kIntScale * unit[axis]);  // checked: a unit component, so within 256 either way
        const auto step = static_cast<std::uint32_t>(point[axis] < 0.0F ? -ulps : ulps);  // checked: an int of at most 256, wrapped by design
        out[axis] = glm::uintBitsToFloat(glm::floatBitsToUint(point[axis]) + step);
    }
    return out;
}

};  // namespace v3d::moya::kernel::geometry
