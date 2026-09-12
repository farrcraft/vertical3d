/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseButton.h"

#include <string>
#include <string_view>

namespace v3d::event::kind {
/**
 **/
MouseButton::MouseButton(const std::string& button, const glm::vec2& position,
    const boost::shared_ptr<Context>& context, bool pressed) :
    Event(button, context),
    position_(position),
    pressed_(pressed) {
    // the edge belongs in the event's state; data is reserved for a parameter
    state(pressed ? State::Pressed : State::Released);
}

/**
 **/
std::string_view MouseButton::button() const {
    return name();
}

/**
 **/
glm::vec2 MouseButton::position() const noexcept {
    return position_;
}

/**
**/
bool MouseButton::pressed() const noexcept {
    return pressed_;
}

};  // namespace v3d::event::kind
