/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::volume {

/** Where a grid's buffer lies among the blocks: its first block and how many it takes. **/
struct GridPlace {
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
};

};  // namespace v3d::moya::kernel::volume
