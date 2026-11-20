/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>

#include <cstdint>

namespace v3d::moya::kernel::light {

/**
 * A light drawn at a shading point: its place among the lights, the triangle drawn of a mesh
 * light, and the chance of drawing both. A place of kNoPrimitive draws nothing.
 **/
struct Picked {
    std::uint32_t place { geometry::kNoPrimitive };
    std::uint32_t primitive { geometry::kNoPrimitive };
    float chance { 0.0F };
};

};  // namespace v3d::moya::kernel::light
