/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::texture {

/**
 * How a lookup filters, as MaterialX's image node names it. The numbers are the order of
 * MaterialX's enumeration.
 **/
enum class Filtering : std::uint8_t {
    Closest,  /**< the texel of the first level the coordinate lies in **/
    Linear,   /**< over the footprint, through the mip levels **/
    Cubic     /**< as Linear **/
};

};  // namespace v3d::moya::kernel::texture
