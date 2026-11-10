/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::surface {

/** The number a geometry names its `N` primvar by: a normal, in the geometry's own space. **/
inline constexpr std::uint32_t kNormal = 0;

/** The number a geometry names its `Cs` primvar by: a colour, in ACEScg. **/
inline constexpr std::uint32_t kColour = 1;

/**
 * A primvar of a geometry, as shading reads it: the number of its name, its floats per value, and
 * where its values start.
 *
 * A primvar of no values per primitive holds one value for the whole geometry. Any other holds
 * that many values for each primitive, one after another: one for a primitive's own value, and
 * three for a triangle's corners, in the order of its points.
 **/
struct Primvar {
    std::uint32_t name { 0 };
    std::uint32_t components { 0 };
    std::uint32_t first { 0 };
    std::uint32_t perPrimitive { 0 };
};

};  // namespace v3d::moya::kernel::surface
