/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Key.h"

#include <string>

namespace v3d::event {
/**
 **/
Key::Key(const std::string& name, const boost::shared_ptr<Context>& context, bool pressed) noexcept :
    Event(name, context),
    pressed_(pressed) {
    // the edge belongs in the event's state; data is reserved for a parameter
    state(pressed ? State::Pressed : State::Released);
}

/**
**/
bool Key::pressed() const noexcept {
    return pressed_;
}

};  // namespace v3d::event
