/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Device.h"
#include "KeyState.h"

namespace v3d::input {
    /**
     **/
    class Keyboard final : public Device {
     public:
        /**
         * Inherit base constructor
         **/
        using Device::Device;

        /**
         * @return bool true if the event was handled
         **/
        bool handleEvent(const SDL_Event& event);

     private:
        KeyState state_;
    };
};  // namespace v3d::input
