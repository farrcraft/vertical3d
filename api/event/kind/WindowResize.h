/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::event::kind {

/**
 * A window changed size. It carries the window's id, as Window::id() gives it, so an app with
 * more than one window can tell which. An engine with no window of its own reports 0.
 **/
class WindowResize final {
 public:
    /**
     **/
    explicit WindowResize(const glm::ivec2& size, std::uint32_t window = 0) noexcept;
    int width() const noexcept;
    int height() const noexcept;
    std::uint32_t window() const noexcept;

 private:
     int width_;
     int height_;
     std::uint32_t window_;
};
};  // namespace v3d::event::kind
