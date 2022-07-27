/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <SDL.h>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

#include <vector>

#include "Device.h"

namespace v3d::input {
    /**
     * The Input Engine is responsible for handling input from supported/registered
     * devices, e.g. mouse, keyboard.
     **/
    class Engine final {
     public:
        /**
         **/
        Engine(const boost::shared_ptr<entt::dispatcher> &dispatcher, int devices);

        /**
         **/
        bool filterEvent(const SDL_Event& event);

     private:
        std::vector<boost::shared_ptr<Device> > devices_;
        boost::shared_ptr<entt::dispatcher> dispatcher_;
    };

};  // namespace v3d::input
