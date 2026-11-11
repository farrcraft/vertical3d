/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Tally.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

namespace v3d::moya::device::cpu {

/**
 * The CPU's traversal, counting each ray it traces and the ticks it takes into one pixel's tally.
 * The work item that owns the pixel is its only writer, so no two threads share a tally.
 *
 * Only the CPU's libraries define nearest() and any(), as they do the traversal's.
 **/
struct Profiled {
    Traversal traversal;
    Tally * tally { nullptr };

    kernel::geometry::Hit nearest(const kernel::geometry::Ray & ray) const;
    bool any(const kernel::geometry::Ray & ray) const;
};

};  // namespace v3d::moya::device::cpu
