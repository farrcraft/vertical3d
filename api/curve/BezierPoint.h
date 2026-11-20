/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * How a Bezier point's handle is placed.
 *
 * Automatic and Vector handles are placed from the points around them, and settle() places
 * them again whenever a point moves. Aligned and Free handles stay where they were put.
 **/
enum class Handle : std::uint8_t {
    Free,       /**< where it was put, in any direction **/
    Aligned,    /**< where it was put, in line with the point's other handle **/
    Vector,     /**< a third of the way toward the neighbouring point on its side **/
    Automatic   /**< along the line through the neighbours, so the curve is smooth there **/
};

/**
 * A point a Bezier curve passes through, and the two handles that shape it.
 *
 * left shapes the segment that arrives at the point, and right the segment that leaves it.
 **/
struct BezierPoint final {
    glm::vec3 position{ 0.0F };
    glm::vec3 left{ 0.0F };
    glm::vec3 right{ 0.0F };
    Handle leftType = Handle::Automatic;
    Handle rightType = Handle::Automatic;

    bool operator==(const BezierPoint& other) const = default;
};

};  // namespace v3d::curve
