/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mouse.h"

#include <api/event/Source.h>
#include <api/event/State.h>
#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/MouseWheel.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>

namespace v3d::input {

namespace {

/**
 * Map an SDL mouse button index to its name, or to an empty string for a button with none.
 * A binding config names a mouse button by this, as it names a key by keyName().
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

};  // namespace

/**
 **/
bool isButtonName(std::string_view name) {
    const std::initializer_list<unsigned int> buttons{SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1, SDL_BUTTON_X2};
    return std::ranges::any_of(buttons, [name](unsigned int button) { return buttonEvent(button) == name; });
}

bool Mouse::handleEvent(const SDL_Event& event) {
    std::string buttonName;
    bool pressed = true;
    switch (event.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        pressed = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        buttonName = buttonEvent(event.button.button);
        // a button with no name cannot be bound, and is kept out of MouseState. There it
        // would be held under an empty name that no caller can query
        if (buttonName.empty()) {
            return true;
        }
        if (state_.held(buttonName) != pressed) {
            state_(buttonName);
        }
        const glm::vec2 position(event.button.x, event.button.y);
        state_(position);
        dispatcher()->trigger<v3d::event::kind::MouseButton>(
            v3d::event::kind::MouseButton(buttonName, position, context(), pressed));
        break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        const glm::vec2 position(event.motion.x, event.motion.y);
        state_(position);
        dispatcher()->trigger<v3d::event::kind::MouseMotion>(
            v3d::event::kind::MouseMotion({.position = position, .motion = glm::vec2(event.motion.xrel, event.motion.yrel)}, context()));
        return true;  // motion has no discrete name, so it is not a bindable source event
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        // a wheel sends one event per notch. MouseState adds them up, and the frame reads
        // the total for the whole flick
        const glm::vec2 notches(event.wheel.x, event.wheel.y);
        state_.wheel(notches.y);
        dispatcher()->trigger<v3d::event::kind::MouseWheel>(
            v3d::event::kind::MouseWheel({.notches = notches, .position = glm::vec2(event.wheel.mouse_x, event.wheel.mouse_y)}, context()));
        return true;  // a turn has no discrete name either, so nothing binds to one
    }
    default:
        return false;
    }

    // publish a source event for the bindings to map, as the keyboard does. The edge is the
    // event's state, not its data.
    v3d::event::publish(*dispatcher(), v3d::event::Source(buttonName, context(),
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
