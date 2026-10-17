/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowResize.h"

namespace v3d::event::kind {

WindowResize::WindowResize(const glm::ivec2& size) noexcept : width_(size.x), height_(size.y) {
}

int WindowResize::width() const noexcept {
    return width_;
}

int WindowResize::height() const noexcept {
    return height_;
}
};  // namespace v3d::event::kind
