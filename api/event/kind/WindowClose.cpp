/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowClose.h"

#include <cstdint>

namespace v3d::event::kind {

WindowClose::WindowClose(std::uint32_t window) noexcept : window_(window) {
}

std::uint32_t WindowClose::window() const noexcept {
    return window_;
}
};  // namespace v3d::event::kind
