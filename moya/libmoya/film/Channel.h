/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>

namespace v3d::moya::film {

/** A channel a film holds beside the beauty: an output such as albedo, normal or depth. **/
struct Channel {
    std::string name;

    /** Its floats per pixel, from 1 to 4. **/
    std::uint32_t components { 1 };
};

};  // namespace v3d::moya::film
