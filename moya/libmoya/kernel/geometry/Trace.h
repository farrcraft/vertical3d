/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::geometry {

/**
 * What tracing a list of rays reads and writes. The rays are laid over an image of a width, so a
 * launch reaches each. Each ray gets its nearest hit, and whether anything lies on it.
 **/
template <Traversal T>
struct TraceRays {
    T traversal;
    Array<Ray> rays;
    Array<Hit> nearest;
    Array<std::uint32_t> any;
    std::uint32_t width { 1 };
};

/** Traces one item's ray: its nearest hit, and one or zero for whether anything lies on it. **/
template <Traversal T>
MOYA_KERNEL void traceRays(const TraceRays<T> & params, Item item) {
    const std::uint32_t at = (item.y * params.width) + item.x;
    if (at >= params.rays.size) {
        return;
    }
    params.nearest[at] = params.traversal.nearest(params.rays[at]);
    params.any[at] = params.traversal.any(params.rays[at]) ? 1U : 0U;
}

};  // namespace v3d::moya::kernel::geometry
