/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * A point on a cubic Bezier segment from start to end, shaped by the two handles, at t from 0
 * to 1. It is de Casteljau's construction: three rounds of interpolating between neighbours.
 **/
inline glm::vec3 bezierAt(const glm::vec3& start, const glm::vec3& leave, const glm::vec3& arrive, const glm::vec3& end, float t) {
    const glm::vec3 first = glm::mix(start, leave, t);
    const glm::vec3 middle = glm::mix(leave, arrive, t);
    const glm::vec3 last = glm::mix(arrive, end, t);
    return glm::mix(glm::mix(first, middle, t), glm::mix(middle, last, t), t);
}

/**
 * A point on a uniform Catmull-Rom segment from start to end, whose neighbours are before and
 * after, at t from 0 to 1.
 *
 * The segment is the Bezier segment whose handles lie a sixth of the way from one neighbour to
 * the other, so that its slope at each end is half the step between that end's neighbours.
 **/
inline glm::vec3 catmullRomAt(const glm::vec3& before, const glm::vec3& start, const glm::vec3& end, const glm::vec3& after, float t) {
    constexpr float SIXTH = 1.0F / 6.0F;
    return bezierAt(start, start + ((end - before) * SIXTH), end - ((after - start) * SIXTH), end, t);
}

};  // namespace v3d::curve
