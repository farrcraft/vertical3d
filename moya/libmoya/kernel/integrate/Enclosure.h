/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/**
 * A closed surface a path is inside: the key of the chain of instances that placed it, its
 * priority, and the index of refraction of the medium it encloses.
 **/
struct Enclosure {
    std::uint32_t key { 0 };
    std::int32_t priority { 0 };
    float index { 1.0F };
};

};  // namespace v3d::moya::kernel::integrate
