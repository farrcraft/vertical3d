/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>

#include <vector>

#include <glm/vec2.hpp>

namespace v3d::brep::internal {

/**
 * A face's corners counter-clockwise seen from outside, and where each lands in the face's own
 * plane.
 **/
struct Outline {
    std::vector<Index> corners;
    std::vector<glm::vec2> flat;
};

};  // namespace v3d::brep::internal
