/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "DeviceType.h"

#include <boost/make_shared.hpp>

namespace v3d::input {

    /**
     **/
    Engine::Engine(const boost::shared_ptr<entt::dispatcher>& dispatcher, int devices) :
        dispatcher_(dispatcher) {

        // add keyboard & mouse devices
        if (devices & DeviceType::Keyboard) {
            devices_.push_back(boost::make_shared<Keyboard>(dispatcher_));
        }
        if (devices & DeviceType::Mouse) {
            devices_.push_back(boost::make_shared<Mouse>(dispatcher_));
        }
    }

    /**
     **/
    bool Engine::filterEvent(const SDL_Event& event) {
        // iterate over devices
        for (auto it = devices_.begin(); it != devices_.end(); it++) {
            if ((*it)->handleEvent(event)) {
                return true;
            }
        }
        return false;
    }

};  // namespace v3d::input
