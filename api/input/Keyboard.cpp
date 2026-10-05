/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keyboard.h"

#include <api/event/Source.h>
#include <api/event/kind/KeyDown.h>
#include <api/event/kind/KeyUp.h>
#include <api/event/kind/TextInput.h>

#include <algorithm>
#include <string>
#include <string_view>

namespace v3d::input {

namespace {

/**
 * Every key this library has a name for, and the name. Read in both directions: an SDL key
 * to the name a binding and a text box use, and a name back to whether it is one.
 **/
struct Named {
    SDL_Keycode key;
    std::string_view name;
};

constexpr Named KEYS[] = {
    {SDLK_ESCAPE, "escape"},
    {SDLK_RETURN, "return"},
    {SDLK_A, "a"},
    {SDLK_B, "b"},
    {SDLK_C, "c"},
    {SDLK_D, "d"},
    {SDLK_E, "e"},
    {SDLK_F, "f"},
    {SDLK_G, "g"},
    {SDLK_H, "h"},
    {SDLK_I, "i"},
    {SDLK_J, "j"},
    {SDLK_K, "k"},
    {SDLK_L, "l"},
    {SDLK_M, "m"},
    {SDLK_N, "n"},
    {SDLK_O, "o"},
    {SDLK_P, "p"},
    {SDLK_Q, "q"},
    {SDLK_R, "r"},
    {SDLK_S, "s"},
    {SDLK_T, "t"},
    {SDLK_U, "u"},
    {SDLK_V, "v"},
    {SDLK_W, "w"},
    {SDLK_X, "x"},
    {SDLK_Y, "y"},
    {SDLK_Z, "z"},
    {SDLK_0, "0"},
    {SDLK_1, "1"},
    {SDLK_2, "2"},
    {SDLK_3, "3"},
    {SDLK_4, "4"},
    {SDLK_5, "5"},
    {SDLK_6, "6"},
    {SDLK_7, "7"},
    {SDLK_8, "8"},
    {SDLK_9, "9"},
    {SDLK_SLASH, "/"},
    {SDLK_PERIOD, "."},
    {SDLK_MINUS, "-"},
    {SDLK_COMMA, ","},
    {SDLK_SEMICOLON, ";"},
    {SDLK_EQUALS, "="},
    {SDLK_APOSTROPHE, "'"},
    {SDLK_LEFTBRACKET, "["},
    {SDLK_RIGHTBRACKET, "]"},
    {SDLK_BACKSLASH, "\\"},
    {SDLK_CAPSLOCK, "capslock"},
    {SDLK_TAB, "tab"},
    {SDLK_GRAVE, "`"},
    {SDLK_UP, "arrow_up"},
    {SDLK_DOWN, "arrow_down"},
    {SDLK_LEFT, "arrow_left"},
    {SDLK_RIGHT, "arrow_right"},
    {SDLK_F1, "f1"},
    {SDLK_F2, "f2"},
    {SDLK_F3, "f3"},
    {SDLK_F4, "f4"},
    {SDLK_F5, "f5"},
    {SDLK_F6, "f6"},
    {SDLK_F7, "f7"},
    {SDLK_F8, "f8"},
    {SDLK_F9, "f9"},
    {SDLK_F10, "f10"},
    {SDLK_F11, "f11"},
    {SDLK_F12, "f12"},
    {SDLK_F13, "f13"},
    {SDLK_F14, "f14"},
    {SDLK_F15, "f15"},
    {SDLK_LALT, "left_alt"},
    {SDLK_RALT, "right_alt"},
    {SDLK_RCTRL, "right_control"},
    {SDLK_LCTRL, "left_control"},
    {SDLK_RSHIFT, "right_shift"},
    {SDLK_LSHIFT, "left_shift"},
    {SDLK_INSERT, "insert"},
    {SDLK_HOME, "home"},
    {SDLK_DELETE, "delete"},
    {SDLK_PAGEUP, "pageup"},
    {SDLK_PAGEDOWN, "pagedown"},
    {SDLK_SPACE, "space"},
    {SDLK_BACKSPACE, "backspace"},
    {SDLK_END, "end"},
};

};  // namespace

/**
 **/
std::string keyName(SDL_Keycode key) {
    for (const Named& named : KEYS) {
        if (named.key == key) {
            return std::string(named.name);
        }
    }
    return std::string();
}

/**
 **/
bool isKeyName(std::string_view name) {
    return std::ranges::any_of(KEYS, [name](const Named& named) { return named.name == name; });
}

/**
 **/
void Keyboard::flush() {
    state_.flush();
}

/**
 **/
const KeyState& Keyboard::state() const {
    return state_;
}

/**
 **/
bool Keyboard::handleEvent(const SDL_Event& event) {
    std::string name;
    bool pressed = true;
    switch (event.type) {
    case SDL_EVENT_TEXT_INPUT:
        // what the platform composed rather than which key moved, because the two are not
        // the same question - ADR-0040. It is not a source event: nothing binds a command
        // to a letter being typed
        dispatcher_->trigger<v3d::event::kind::TextInput>(v3d::event::kind::TextInput(event.text.text, context_));
        return true;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        pressed = (event.type == SDL_EVENT_KEY_DOWN);
        name = keyName(event.key.key);
        // a key we have no name for cannot be bound to anything, and must not reach
        // KeyState either - it would be held under an empty name that nothing can ask for
        if (name.empty()) {
            return true;
        }
        if (state_.held(name) != pressed) {
            state_(name);
        }
        if (pressed) {
            dispatcher_->trigger<v3d::event::kind::KeyDown>(v3d::event::kind::KeyDown(name, context_));
        } else {
            dispatcher_->trigger<v3d::event::kind::KeyUp>(v3d::event::kind::KeyUp(name, context_));
        }
        break;
    default:
        return false;
    }

    // trigger an event source event so any mappers can propogate any mapped events.
    // the edge is carried as the event's state, not as its data - data is the binding's
    // parameter, and the two would otherwise overwrite each other.
    v3d::event::publish(*dispatcher_, v3d::event::Source(name, context_,
        pressed ? v3d::event::State::Pressed : v3d::event::State::Released));

    return true;
}

};  // namespace v3d::input
