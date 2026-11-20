/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::texture {

/**
 * How a lookup reads beyond a texture's edge along one axis, as MaterialX's image node names it.
 * The numbers are the order of MaterialX's enumeration.
 **/
enum class Address : std::uint8_t {
    Constant,  /**< a coordinate outside zero to one gives the lookup's default **/
    Clamp,     /**< a texel beyond the edge is the nearest edge texel **/
    Periodic,  /**< the texture repeats **/
    Mirror     /**< the texture repeats, every other copy reflected **/
};

};  // namespace v3d::moya::kernel::texture
