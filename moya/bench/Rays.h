/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Ray.h>

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::bench {

/**
 * Coherent rays: a square of side by side rays straight down -z from above a box, spread evenly
 * across its x and y, as a camera's rays are. Each is at the middle of the shutter.
 **/
std::vector<kernel::geometry::Ray> coherent(const glm::vec3 & low, const glm::vec3 & high, std::uint32_t side);

/**
 * Incoherent rays: a count of rays from points drawn evenly inside a box, in directions drawn
 * evenly over the sphere, at times drawn evenly over the shutter, as a path tracer's bounces are.
 * The same seed draws the same rays.
 **/
std::vector<kernel::geometry::Ray> incoherent(const glm::vec3 & low, const glm::vec3 & high, std::uint32_t count, std::uint32_t seed);

};  // namespace v3d::moya::bench
