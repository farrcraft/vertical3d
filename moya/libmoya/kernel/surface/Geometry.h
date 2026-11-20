/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>

#include <cstdint>

namespace v3d::moya::kernel::surface {

/**
 * A geometry as shading reads it: the form of its primitives, where their values are held at each
 * sample, its first face or curve, and its run of primvars. It holds what the gather's geometry
 * holds, in kernel code's own type.
 **/
struct Geometry {
    geometry::primitive::Form form { geometry::primitive::Form::Triangles };
    geometry::Sampled points;
    geometry::Sampled widths;
    geometry::Sampled quadric;
    geometry::Sampled controls;
    std::uint32_t ends { 0 };

    /** The first of a mesh's faces or of curves' curves. **/
    std::uint32_t faces { 0 };

    /** The first of its primvars, and how many it has. **/
    std::uint32_t primvars { 0 };
    std::uint32_t primvarCount { 0 };
};

};  // namespace v3d::moya::kernel::surface
