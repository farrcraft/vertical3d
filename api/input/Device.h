/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <entt/entt.hpp>
#include <boost/shared_ptr.hpp>
#include <SDL.h>

namespace v3d::input {
    /**
     * Base for input devices
     **/
    class Device {
     public:
        /**
         **/
        Device(const boost::shared_ptr<entt::dispatcher> &dispatcher);

        /**
         **/
        virtual ~Device() = default;

        /**
         * Handle any device events
         * If the event is associated with this device, true should always be returned.
         * 
         * @param event a potential event to handle
         * 
         * @return true if the event was handled by this device
         **/
        virtual bool handleEvent(const SDL_Event& event) = 0;

     protected:
        boost::shared_ptr<entt::dispatcher> dispatcher_;
    };
};  // namespace v3d::input
