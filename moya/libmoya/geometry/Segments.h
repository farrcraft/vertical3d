/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec4.hpp>

namespace v3d::moya::geometry {

/**
 * A set of curves cut into segments. Each segment's control points are a centre in x, y and z
 * and a radius in w: two to a linear segment, and four to a cubic one, as a Bézier curve's.
 **/
struct Segments {
    std::vector<glm::vec4> controls;

    /** The curve each segment was cut from, one per segment. **/
    std::vector<std::uint32_t> curves;

    /** One for each segment that closes its curve, and zero for one the next carries on from. **/
    std::vector<std::uint32_t> ends;
};

};  // namespace v3d::moya::geometry
