/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <type_traits>

namespace v3d::moya::kernel::geometry {

/**
 * What a stage that traces rays takes as its traversal. Each device instantiates the stages with
 * its own, so kernel code names no device, and a test runs the same stages over any traversal.
 *
 * nearest(ray) is the hit nearest the ray's start within its interval, or a hit that found
 * nothing. any(ray) is whether anything lies within the interval, and may stop at the first
 * primitive it meets, as a shadow ray needs. Both are const, since a launch traces from every
 * pixel at once.
 **/
template <typename T>
concept Traversal = requires(const T & traversal, const Ray & ray) {
    requires std::is_same_v<decltype(traversal.nearest(ray)), Hit>;
    requires std::is_same_v<decltype(traversal.any(ray)), bool>;
};

};  // namespace v3d::moya::kernel::geometry
