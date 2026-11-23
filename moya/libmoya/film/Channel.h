/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/film/Combine.h>

#include <cstdint>
#include <string>

namespace v3d::moya::film {

/** A channel a film holds beside the beauty: an output such as albedo, normal or depth. **/
struct Channel {
    std::string name;

    /** Its components, from 1 to 4. **/
    std::uint32_t components { 1 };

    /** How its samples are combined into its pixel. **/
    kernel::film::Combine combine { kernel::film::Combine::Mean };

    bool operator==(const Channel &) const = default;
};

};  // namespace v3d::moya::film
