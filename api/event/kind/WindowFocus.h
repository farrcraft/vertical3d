/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::event::kind {

/**
 * The window gained or lost keyboard focus.
 *
 * An app uses it to pause, to mute, and to drop held input. A key released while the window
 * is unfocused sends no release, so the key reads as held until the app handles the focus loss.
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
