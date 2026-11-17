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
 * encloses.
 **/
struct Enclosure {
    std::uint32_t key { 0 };
    std::int32_t priority { 0 };
    float index { 1.0F };
    std::uint32_t material { 0 };
};

};  // namespace v3d::moya::kernel::integrate::media
