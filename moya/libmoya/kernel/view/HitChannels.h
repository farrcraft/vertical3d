/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::view {

/** Where each of the hit view's channels starts in a pixel, as the film gives them. **/
struct HitChannels {
    std::uint32_t depth { 0 };
    std::uint32_t normal { 0 };
    std::uint32_t instance { 0 };
    std::uint32_t face { 0 };
    std::uint32_t coverage { 0 };
};

};  // namespace v3d::moya::kernel::view
