/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <algorithm>
#include <limits>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

/**
 * The bounding volume hierarchies rays are traversed through, built on the host from the scene's
 * primitives and copied to a device.
 **/
namespace v3d::moya::accel {

/** An axis-aligned box. An empty box holds nothing, and growing it by anything gives that. **/
struct Box {
    glm::vec3 lower { std::numeric_limits<float>::infinity() };
    glm::vec3 upper { -std::numeric_limits<float>::infinity() };

    bool empty() const {
        return lower.x > upper.x || lower.y > upper.y || lower.z > upper.z;
    }

    void grow(const glm::vec3 & point) {
        lower = glm::min(lower, point);
        upper = glm::max(upper, point);
    }

    void grow(const Box & other) {
        lower = glm::min(lower, other.lower);
        upper = glm::max(upper, other.upper);
    }

    /** The box's surface area, zero for an empty one. **/
    float area() const {
        if (empty()) {
            return 0.0F;
        }
        const glm::vec3 size = upper - lower;
        return 2.0F * ((size.x * size.y) + (size.y * size.z) + (size.z * size.x));
    }

    glm::vec3 centre() const {
        return (lower + upper) * 0.5F;
    }

    /** The part of this box inside another. **/
    Box within(const Box & other) const {
        return Box { .lower = glm::max(lower, other.lower), .upper = glm::min(upper, other.upper) };
    }

    /** Whether this box holds another, which an empty one is held by every box. **/
    bool holds(const Box & other) const {
        return other.empty() || (glm::all(glm::lessThanEqual(lower, other.lower)) && glm::all(glm::greaterThanEqual(upper, other.upper)));
    }

    /** The axis the box is longest along. **/
    int longest() const {
        const glm::vec3 size = upper - lower;
        if (size.x >= std::max(size.y, size.z)) {
            return 0;
        }
        return size.y >= size.z ? 1 : 2;
    }
};

};  // namespace v3d::moya::accel
