/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/scene/Scene.h>

#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::moya::geometry {

/**
 * A transformation decomposed at a time, by polar decomposition: its translation, the rotation
 * nearest its linear part, and the symmetric scale left over. A linear part that mirrors keeps a
 * rotation, and its scale mirrors. One that flattens space has no rotation, and the whole linear
 * part is its scale.
 **/
kernel::geometry::Key decompose(const glm::mat4 & transform, float time);

/**
 * The keys of a transformation sampled over the shutter, one at each of its times. Each rotation
 * is the one of its sign nearer the rotation before it, so that each is interpolated the shortest
 * way.
 **/
std::vector<kernel::geometry::Key> keys(const scene::Samples & samples);

};  // namespace v3d::moya::geometry
