/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/interchange/Scene.h>

namespace v3d::interchange {

/**
 * Multiply every length a scene holds by a factor, so a scene read in metres can be put into a
 * scene measured in another unit.
 *
 * The lengths are a mesh's points, a node's position, a translation channel's values and
 * tangents, a curve's points and handles and its width, and a camera's clipping planes and
 * orthographic extent. Rotations, scales, angles, a NURBS curve's weights and knots, and a
 * light's power are not lengths, and are kept.
 **/
void rescale(Scene* scene, float factor);

};  // namespace v3d::interchange
