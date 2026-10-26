/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Devices.h"

#include <api/event/Context.h>
#include <api/event/Router.h>
#include "Device.h"
#include "KeyState.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "DeviceType.h"
#include "MouseState.h"

#include <boost/make_shared.hpp>

namespace v3d::input {

/**
 **/
Devices::Devices(const boost::shared_ptr<v3d::event::Router>& router, const boost::shared_ptr<entt::dispatcher>& dispatcher, DeviceTypes devices) :
    dispatcher_(dispatcher) {
    if (devices.has(DeviceType::Keyboard)) {
        const boost::shared_ptr<v3d::event::Context> keyboardContext = router->resolveContext("keyboard");
        keyboard_ = boost::make_shared<Keyboard>(keyboardContext, dispatcher_);
        devices_.push_back(keyboard_);
    }
    if (devices.has(DeviceType::Mouse)) {
        const boost::shared_ptr<v3d::event::Context> mouseContext = router->resolveContext("mouse");
        mouse_ = boost::make_shared<Mouse>(mouseContext, dispatcher_);
        devices_.push_back(mouse_);
    }
}

/**
 **/
void Devices::flush() {
    for (const boost::shared_ptr<Device>& device : devices_) {
        device->flush();
    }
}

/**
 **/
const KeyState* Devices::keys() const {
    return keyboard_ ? &keyboard_->state() : nullptr;
}

/**
 **/
const MouseState* Devices::mouse() const {
    return mouse_ ? &mouse_->state() : nullptr;
}

/**
 **/
bool Devices::filterEvent(const SDL_Event& event) {
    for (auto it = devices_.begin(); it != devices_.end(); it++) {
        if ((*it)->handleEvent(event)) {
            return true;
        }
    }
    return false;
}

};  // namespace v3d::input
