/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::event {
/**
 **/
enum class Type : std::uint8_t {
    Unknown = 0,
    Source = 1,
    Destination = 2
};
};  // namespace v3d::event
