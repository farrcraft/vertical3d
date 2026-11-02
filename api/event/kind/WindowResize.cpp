/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowResize.h"

#include <cstdint>

namespace v3d::event::kind {

WindowResize::WindowResize(const glm::ivec2& size, std::uint32_t window) noexcept : width_(size.x), height_(size.y), window_(window) {
}

int WindowResize::width() const noexcept {
    return width_;
}

int WindowResize::height() const noexcept {
    return height_;
}

std::uint32_t WindowResize::window() const noexcept {
    return window_;
}
};  // namespace v3d::event::kind
