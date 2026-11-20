/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * A curve of straight lines through its points.
 **/
struct Poly final {
    std::vector<glm::vec3> points;

    bool operator==(const Poly& other) const = default;
};

};  // namespace v3d::curve
