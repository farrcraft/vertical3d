/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mouse.h"

#include <api/event/Source.h>
#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/MouseWheel.h>

#include <string>
#include <string_view>

namespace v3d::input {

/**
 * Map an SDL mouse button index to a string representation.
 * These are the names a binding config uses, the way key names are for the keyboard.
 **/
std::string buttonEvent(unsigned int button) {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return "left";
    case SDL_BUTTON_MIDDLE:
        return "middle";
    case SDL_BUTTON_RIGHT:
        return "right";
    case SDL_BUTTON_X1:
        return "x1";
    case SDL_BUTTON_X2:
        return "x2";
    default:
        break;
    }
    return std::string();
}

/**
 **/
bool isButtonName(std::string_view name) {
    for (unsigned int button : {SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1, SDL_BUTTON_X2}) {
        if (buttonEvent(button) == name) {
            return true;
        }
    }
    return false;
}

bool Mouse::handleEvent(const SDL_Event& event) {
    std::string buttonName;
    bool pressed = true;
    switch (event.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        pressed = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        buttonName = buttonEvent(event.button.button);
        // a button we have no name for cannot be bound to anything, and must not reach
        // MouseState either - it would be held under an empty name that nothing can ask for
        if (buttonName.empty()) {
            return true;
        }
        if (state_.held(buttonName) != pressed) {
            state_(buttonName);
        }
        const glm::vec2 position(event.button.x, event.button.y);
        state_(position);
        dispatcher_->trigger<v3d::event::kind::MouseButton>(
            v3d::event::kind::MouseButton(buttonName, position, context_, pressed));
        break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        glm::vec2 position(event.motion.x, event.motion.y);
        state_(position);
        dispatcher_->trigger<v3d::event::kind::MouseMotion>(
            v3d::event::kind::MouseMotion(position, glm::vec2(event.motion.xrel, event.motion.yrel), context_));
        return true;  // motion is not a bindable source event - it has no discrete name
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        // a wheel sends one event per notch, so the state accumulates them and the frame
        // reads what the whole flick came to
        const glm::vec2 notches(event.wheel.x, event.wheel.y);
        state_.wheel(notches.y);
        dispatcher_->trigger<v3d::event::kind::MouseWheel>(
            v3d::event::kind::MouseWheel(notches, glm::vec2(event.wheel.mouse_x, event.wheel.mouse_y), context_));
        return true;  // a turn has no discrete name either, so nothing binds to one
    }
    default:
        return false;
    }

    // trigger a source event so any mappers can propagate a mapped event, the same way
    // the keyboard does. The edge is the event's state, not its data.
    v3d::event::publish(*dispatcher_, v3d::event::Source(buttonName, context_,
        pressed ? v3d::event::State::Pressed : v3d::event::State::Released));

    return true;
}

/**
 **/
void Mouse::flush() {
    state_.flush();
}

/**
 **/
const MouseState& Mouse::state() const {
    return state_;
}

};  // namespace v3d::input
