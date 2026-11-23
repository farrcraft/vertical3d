/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * A smooth curve through every one of its points, each segment shaped by the points either side
 * of it.
 *
 * It is uniform Catmull-Rom. An open curve's two end segments treat the end point as its own
 * neighbour, so the curve starts and ends on its first and last points.
 **/
struct CatmullRom final {
    std::vector<glm::vec3> points;

    bool operator==(const CatmullRom& other) const = default;
};

};  // namespace v3d::curve
