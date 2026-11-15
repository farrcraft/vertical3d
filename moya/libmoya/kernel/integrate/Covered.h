/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/** An index a camera sample's record names when it met no surface, or a surface of no material. **/
inline constexpr std::uint32_t kUncovered = 0xFFFFFFFFU;

/**
 * What one camera sample covers, for Cryptomatte: where its camera ray first met a surface, and
 * its filter weight. object is the innermost instance that placed the surface and asset the
 * outermost, by their indices in the scene, and material the surface's material among the
 * surfaces'. A sample that met no surface, or met a light's shape, covers nothing and keeps its
 * weight. A sample a stopped pixel did not take has a weight of zero.
 **/
struct Covered {
    std::uint32_t object { kUncovered };
    std::uint32_t asset { kUncovered };
    std::uint32_t material { kUncovered };
    float weight { 0.0F };
};

};  // namespace v3d::moya::kernel::integrate
