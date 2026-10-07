/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

#include <glm/gtc/quaternion.hpp>

namespace v3d::ecs::component {

/**
 * Where a thing stands in a 3D world, which way it is turned, and how large it is.
 *
 * The position is the thing's origin: a sprite's feet, a mesh's own origin. The fields are
 * written directly, since simulation sets them every step. The value is type::Transform, which
 * an editor mesh is placed by too.
 **/
using Transform = v3d::type::Transform;

/**
 * A turn about +Y, for a world that turns about one axis. A positive angle turns +Z towards
 * +X.
 **/
glm::quat aboutY(float radians);

/**
 * The transform alpha of the way from one to the other, which ecs::interpolated uses to draw
 * it between two simulation steps.
 **/
using v3d::type::interpolate;

};  // namespace v3d::ecs::component
