/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::geometry {

/**
 * One chain of instances that places a geometry in the world, and the run of placed primitives it
 * owns. Its visibility holds the kinds of ray every instance of the chain is visible to.
 **/
struct Placement {
    std::uint32_t instances[kInstanceDepth] {};
    std::uint32_t depth { 0 };

    /** The first of its primitives, and how many there are. **/
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };

    std::uint32_t visibility { kEveryRay };

    /** Whether a hit names this chain. **/
    MOYA_KERNEL bool places(const Hit & hit) const {
        if (depth != hit.depth || hit.primitive >= count) {
            return false;
        }
        bool same = true;
        for (std::uint32_t level = 0; level < hit.depth; ++level) {
            same = same && instances[level] == hit.instances[level];
        }
        return same;
    }
};

};  // namespace v3d::moya::kernel::geometry
