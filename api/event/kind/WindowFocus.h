/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::event::kind {

/**
 * The window gained or lost keyboard focus.
 *
 * Auto-pause, muting and dropping held input all depend on it, and without it an app would
 * have to poll SDL_GetWindowFlags. A key released while the window is unfocused never
 * arrives, so it stays down until focus loss is handled.
 **/
class WindowFocus final {
 public:
    /**
     **/
    explicit WindowFocus(bool gained) noexcept;
    bool gained() const noexcept;

 private:
     bool gained_;
};
};  // namespace v3d::event::kind
