/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::event::kind {

/**
 * The user asked to close a window an app added with Engine::addWindow(). Closing the app's
 * own window asks to quit instead. The app decides what closing the window means, and removes
 * it with Engine::removeWindow() if it goes.
 **/
class WindowClose final {
 public:
    explicit WindowClose(std::uint32_t window) noexcept;

    /**
     * @return the window's id, as Window::id() gives it
     **/
    std::uint32_t window() const noexcept;

 private:
    std::uint32_t window_;
};
};  // namespace v3d::event::kind
