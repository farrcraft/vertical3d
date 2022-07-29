/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "WindowResize.h"

namespace v3d::event {

    WindowResize::WindowResize(int width, int height) noexcept : width_(width), height_(height) {

    }

    int WindowResize::width() const noexcept {
        return width_;
    }

    int WindowResize::height() const noexcept {
        return height_;
    }
};  // namespace v3d::event
