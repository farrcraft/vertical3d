/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::accel {

/** Each triangle's box, for triangles of three points each. **/
std::vector<Box> triangleBounds(const std::vector<glm::vec3> & points);

/**
 * The bounds of the part of a triangle inside a box, by clipping the triangle to each of the
 * box's six planes. Empty when the triangle misses the box.
 **/
Box clipTriangle(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2, const Box & within);

/** A Clip over triangles of three points each, which reads the points it is given. **/
Clip triangleClip(const std::vector<glm::vec3> & points);

};  // namespace v3d::moya::accel
