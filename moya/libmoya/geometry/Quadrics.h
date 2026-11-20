/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>

#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

/** A sampled number at a time, interpolated linearly between its samples, and held beyond them. **/
float numberAt(const scene::Samples & samples, float time);

/** A sampled point at a time, interpolated as numberAt() interpolates a number. **/
glm::vec3 pointOf(const scene::Samples & samples, float time);

/** One number of a sampled array at a time, interpolated as numberAt() interpolates a number. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, then the time
float numberAt(const scene::Samples & samples, std::size_t index, float time);

/** One point of a sampled array at a time, interpolated as numberAt() interpolates a number. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, then the time
glm::vec3 pointOf(const scene::Samples & samples, std::size_t index, float time);

/**
 * One of the scene's quadrics as it stands at a time, each dimension interpolated between its own
 * samples. RI's angles in degrees become radians.
 **/
kernel::geometry::primitive::Quadric quadricAt(const scene::Scene & scene, const scene::Handle & handle, float time);

};  // namespace v3d::moya::geometry
