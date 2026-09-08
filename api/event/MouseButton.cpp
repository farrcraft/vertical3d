/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseButton.h"

namespace v3d::event {
/**
 **/
MouseButton::MouseButton(unsigned int button, const glm::vec2& position,
    const boost::shared_ptr<Context>& context, bool pressed) :
    Event("button", context),
    button_(button),
    position_(position),
    pressed_(pressed) {
    // the edge belongs in the event's state; data is reserved for a parameter
    state(pressed ? State::Pressed : State::Released);
}

/**
 **/
unsigned int MouseButton::button() const noexcept {
    return button_;
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

};  // namespace v3d::event
