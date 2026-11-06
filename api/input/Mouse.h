/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Device.h"
#include "MouseState.h"

#include <string_view>

namespace v3d::input {

/**
 * Whether a name is one a mouse button is bound by - "left", "middle", "right", "x1", "x2".
 **/
bool isButtonName(std::string_view name);
/**
 * Turns SDL's mouse button, motion and wheel events into this library's events, and tracks the
 * cursor, the held buttons and the wheel.
 **/
class Mouse : public Device {
 public:
    /**
     * Inherit base constructor
     **/
    using Device::Device;

    /**
     * @return bool true if the event was handled
     **/
    bool handleEvent(const SDL_Event& event) override;

    /**
     * The cursor position and held buttons, as of the last event handled.
     **/
    const MouseState& state() const;

    /**
     **/
    void flush() override;

 private:
    MouseState state_;
};
};  // namespace v3d::input
