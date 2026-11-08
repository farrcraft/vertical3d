/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/Shapes.h>

namespace v3d::moya::device::cpu {

/**
 * A geometry as the CPU traverses it: the form of its primitives, and where their values are held
 * at each sample. Triangles read their points, three per triangle. Points read their centres and
 * their widths. A quadric reads its dimensions.
 **/
struct Geometry {
    kernel::geometry::Form form { kernel::geometry::Form::Triangles };
    kernel::geometry::Sampled points;
    kernel::geometry::Sampled widths;
    kernel::geometry::Sampled quadric;
};

};  // namespace v3d::moya::device::cpu
