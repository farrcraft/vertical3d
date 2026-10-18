/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Model.h>

#include <glm/glm.hpp>

namespace v3d::test {

/**
 * A unit cube centred on the origin, a face at a time so that each face has its own normal,
 * wound counter clockwise seen from outside as a glTF model is.
 **/
v3d::type::Model cube(const glm::vec4& colour);

/**
 * Two cubes side by side along x in one model, a part each: red on the left, green on the
 * right. Each cube's centre is 0.75 from the origin.
 **/
v3d::type::Model twoCubes();

};  // namespace v3d::test
