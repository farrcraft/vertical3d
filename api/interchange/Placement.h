/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

#include <glm/mat4x4.hpp>

namespace v3d::interchange {

/**
 * A placement taken from a matrix, for a reader whose format places a node by one.
 *
 * The last column is the translation, the length of each of the other three is a scale, and
 * what is left when those are divided out is the rotation. A matrix that mirrors gives up the
 * mirror to its x scale. A shear cannot be held by a placement, and is lost.
 **/
v3d::type::Transform placementOf(const glm::mat4& matrix);

};  // namespace v3d::interchange
