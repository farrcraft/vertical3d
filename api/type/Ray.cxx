/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Ray.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

namespace v3d::type {

namespace {

/**
 * Below this a direction is degenerate, and a triangle determinant is edge on - a
 * ray running in the plane of the triangle, where the barycentric solution is
 * meaningless.
 **/
constexpr float epsilon = 1e-7f;

};  // namespace

/**
 **/
Ray::Ray() :
    origin_(0.0f, 0.0f, 0.0f),
    direction_(0.0f, 0.0f, 1.0f) {
}

/**
 **/
Ray::Ray(const glm::vec3& origin, const glm::vec3& direction) :
    origin_(origin),
    direction_(direction) {
    const float length = glm::length(direction_);
    if (length > epsilon) {
        direction_ /= length;
    }
}

/**
 **/
const glm::vec3& Ray::origin() const noexcept {
    return origin_;
}

/**
 **/
const glm::vec3& Ray::direction() const noexcept {
    return direction_;
}

/**
 **/
glm::vec3 Ray::point(float distance) const {
    return origin_ + direction_ * distance;
}

/**
 **/
Ray Ray::transformed(const glm::mat4& transform) const {
    Ray moved;
    moved.origin_ = glm::vec3(transform * glm::vec4(origin_, 1.0f));
    moved.direction_ = glm::vec3(transform * glm::vec4(direction_, 0.0f));
    return moved;
}

/**
 **/
bool Ray::intersects(const AABBox& box, float* distance) const {
    const glm::vec3 min = box.min();
    const glm::vec3 max = box.max();

    // not near/far: the windows headers define both as macros
    float entryDistance = 0.0f;
    float exitDistance = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; axis++) {
        if (std::fabs(direction_[axis]) < epsilon) {
            // parallel to this pair of slabs, so it either misses them or is
            // unconstrained by them
            if (origin_[axis] < min[axis] || origin_[axis] > max[axis]) {
                return false;
            }
            continue;
        }
        const float inverse = 1.0f / direction_[axis];
        float entry = (min[axis] - origin_[axis]) * inverse;
        float exit = (max[axis] - origin_[axis]) * inverse;
        if (entry > exit) {
            std::swap(entry, exit);
        }
        entryDistance = std::max(entryDistance, entry);
        exitDistance = std::min(exitDistance, exit);
        if (entryDistance > exitDistance) {
            return false;
        }
    }

    if (distance != nullptr) {
        *distance = entryDistance;
    }
    return true;
}

/**
 **/
bool Ray::intersects(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float* distance) const {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 perpendicular = glm::cross(direction_, ac);
    const float determinant = glm::dot(ab, perpendicular);

    // no back face cull: a modeller picks the inside of a mesh as readily as the
    // outside, so only an edge on ray is rejected here
    if (std::fabs(determinant) < epsilon) {
        return false;
    }

    const float inverse = 1.0f / determinant;
    const glm::vec3 offset = origin_ - a;
    const float u = glm::dot(offset, perpendicular) * inverse;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    const glm::vec3 across = glm::cross(offset, ab);
    const float v = glm::dot(direction_, across) * inverse;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    const float hit = glm::dot(ac, across) * inverse;
    if (hit < 0.0f) {
        return false;
    }

    if (distance != nullptr) {
        *distance = hit;
    }
    return true;
}

};  // namespace v3d::type
