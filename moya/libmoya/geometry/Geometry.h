/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>

#include <cstdint>

namespace v3d::moya::geometry {

/**
 * A geometry's primitives, of one form, and where their values are held at each of their samples.
 * Each reads its own of the samples:
 * - triangles, the points of each triangle, three per triangle, and a run of the faces every
 *   mesh's triangles were cut from, from first.
 * - points, round or flat, each point's centre, and its width in the widths.
 * - a quadric, its dimensions in the quadrics, one per sample.
 * - curves, the control points of each segment, two or four, a run of the curves each segment
 *   was cut from, from first, and a run of which segments close their curves, from ends.
 **/
struct Geometry {
    kernel::geometry::primitive::Form form { kernel::geometry::primitive::Form::Triangles };

    /** The geometry's index in the scene. **/
    std::uint32_t id { 0 };

    /** The first of a mesh's faces or of curves' curves, and how many primitives it holds. **/
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };

    kernel::geometry::Sampled points;
    kernel::geometry::Sampled widths;
    kernel::geometry::Sampled quadric;
    kernel::geometry::Sampled controls;
    std::uint32_t ends { 0 };

    /** Whether the geometry moves over the shutter. **/
    bool moves() const {
        return points.samples > 1 || widths.samples > 1 || quadric.samples > 1 || controls.samples > 1;
    }
};

};  // namespace v3d::moya::geometry
