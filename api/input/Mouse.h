/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Device.h"
#include "MouseState.h"

namespace v3d::input {
/**
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
    bool handleEvent(const SDL_Event& event);

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
