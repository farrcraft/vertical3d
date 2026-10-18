/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

#include <glm/gtc/quaternion.hpp>

namespace v3d::ecs::component {

/**
 * Where an entity stands in a 3D world, which way it is turned, and how large it is.
 *
 * position is the entity's origin, such as a sprite's feet or a mesh's own origin. Simulation
 * writes the fields directly. The type is type::Transform, which also places an editor mesh.
 **/
using Transform = v3d::type::Transform;

/**
 * A turn about +Y, for a world that turns about one axis. A positive angle turns +Z towards
 * +X.
 **/
glm::quat aboutY(float radians);

/**
 * The transform alpha of the way from one to the other. ecs::interpolated() uses it to draw
 * the transform between two simulation steps.
 **/
using v3d::type::interpolate;

};  // namespace v3d::ecs::component
