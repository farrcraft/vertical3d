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
Engine::Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher, int devices) :
    dispatcher_(dispatcher) {
    // add keyboard & mouse devices
    if (devices & DeviceType::Keyboard) {
        boost::shared_ptr<v3d::event::Context> keyboardContext = eventEngine->resolveContext("keyboard");
        keyboard_ = boost::make_shared<Keyboard>(keyboardContext, dispatcher_);
        devices_.push_back(keyboard_);
    }
    if (devices & DeviceType::Mouse) {
        boost::shared_ptr<v3d::event::Context> mouseContext = eventEngine->resolveContext("mouse");
        mouse_ = boost::make_shared<Mouse>(mouseContext, dispatcher_);
        devices_.push_back(mouse_);
    }
}

/**
 **/
void Engine::flush() {
    for (const boost::shared_ptr<Device>& device : devices_) {
        device->flush();
    }
}

/**
 **/
const KeyState* Engine::keys() const {
    return keyboard_ ? &keyboard_->state() : nullptr;
}

/**
 **/
const MouseState* Engine::mouse() const {
    return mouse_ ? &mouse_->state() : nullptr;
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
