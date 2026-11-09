/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>

#include <cstdint>

namespace v3d::moya::device::cpu {

/**
 * A geometry as the CPU traverses it: the form of its primitives, and where their values are held
 * at each sample. Triangles read their points, three per triangle. Points read their centres and
 * their widths. A quadric reads its dimensions. Curves read their segments' control points, and
 * from ends whether each closes its curve.
 **/
struct Geometry {
    kernel::geometry::primitive::Form form { kernel::geometry::primitive::Form::Triangles };
    kernel::geometry::Sampled points;
    kernel::geometry::Sampled widths;
    kernel::geometry::Sampled quadric;
    kernel::geometry::Sampled controls;
    std::uint32_t ends { 0 };
};

};  // namespace v3d::moya::device::cpu
