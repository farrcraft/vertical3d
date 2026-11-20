/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>

namespace v3d::moya::kernel::volume {

/** The number of the primvar a field is read as when no program reads it. **/
inline constexpr std::uint32_t kUnread = std::numeric_limits<std::uint32_t>::max();

/** A field a volume reads: the grid that holds it, and the number of the primvar it is read as. **/
struct Field {
    std::uint32_t grid { 0 };
    std::uint32_t name { kUnread };
};

};  // namespace v3d::moya::kernel::volume
