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
 * What one camera sample covers, for Cryptomatte and deep output: where its camera ray first met
 * a surface, its filter weight, and the light it gathered. object is the innermost instance that
 * placed the surface and asset the outermost, by their indices in the scene, and material the
 * surface's material among the surfaces'. A sample that met no surface, or met a light's shape,
 * covers nothing and keeps its weight. depth is how far along its camera ray it met anything, and
 * infinite for a ray that met nothing. red, green and blue are its radiance in ACEScg. A sample a
 * stopped pixel did not take has a weight of zero.
 *
 * A camera ray that collides in a medium before any surface covers that medium there, and
 * collided is one. Its depth is the collision's. Its object, asset and material are the volume's,
 * or those of the closed surface that encloses the medium.
 **/
struct Covered {
    std::uint32_t object { kUncovered };
    std::uint32_t asset { kUncovered };
    std::uint32_t material { kUncovered };
    float weight { 0.0F };
    float depth { 0.0F };
    float red { 0.0F };
    float green { 0.0F };
    float blue { 0.0F };
    std::uint32_t collided { 0 };
};

};  // namespace v3d::moya::kernel::integrate
