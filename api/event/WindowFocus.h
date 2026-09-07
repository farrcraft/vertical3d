/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::event {

/**
 * The window gained or lost keyboard focus.
 *
 * An app that does not hear this has to poll SDL_GetWindowFlags for something the loop
 * already had in hand, which is what auto-pause, muting and dropping held input are each
 * waiting on - a key released while unfocused never arrives, so it stays down.
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
};  // namespace v3d::event
