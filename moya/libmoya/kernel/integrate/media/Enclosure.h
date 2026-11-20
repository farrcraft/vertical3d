/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::integrate::media {

/**
 * A closed surface a path is inside: the key of the chain of instances that placed it, its
 * priority, the index of refraction of the medium it encloses, and its material, whose medium it
 * encloses. walk is one when the path walks the medium the material's subsurface gives, and zero
 * when it is in the material's own medium. object and asset are the innermost and the outermost
 * instance of the chain, by their indices in the scene, which name the medium for what a camera
 * sample covers.
 **/
struct Enclosure {
    std::uint32_t key { 0 };
    std::int32_t priority { 0 };
    float index { 1.0F };
    std::uint32_t material { 0 };
    std::uint32_t walk { 0 };
    std::uint32_t object { 0 };
    std::uint32_t asset { 0 };
};

};  // namespace v3d::moya::kernel::integrate::media
