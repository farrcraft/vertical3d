/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

/** Which lights light a shading point, and which instances cast a light's shadows. **/
namespace v3d::moya::kernel::light::link {

/**
 * One step from a set of linked lights to another: a shading point whose chain reaches an instance
 * in the set from moves on into the set to, since a light names that instance.
 **/
struct LinkStep {
    std::uint32_t from { 0 };
    std::uint32_t to { 0 };
};

};  // namespace v3d::moya::kernel::light::link
