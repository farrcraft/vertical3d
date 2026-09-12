/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keyboard.h"

#include <api/event/kind/KeyDown.h>
#include <api/event/kind/KeyUp.h>
#include <api/event/kind/TextInput.h>

#include <string>

namespace v3d::input {

/**
 * Map a SDL key symbol to a string representation
 **/
std::string keyEvent(SDL_Keycode key) {
    std::string evnt;

    switch (key) {
    case SDLK_ESCAPE:
        evnt = "escape";
        break;
    case SDLK_RETURN:
        evnt = "return";
        break;
    case SDLK_A:
        evnt = "a";
        break;
    case SDLK_B:
        evnt = "b";
        break;
    case SDLK_C:
        evnt = "c";
        break;
    case SDLK_D:
        evnt = "d";
        break;
    case SDLK_E:
        evnt = "e";
        break;
    case SDLK_F:
        evnt = "f";
        break;
    case SDLK_G:
        evnt = "g";
        break;
    case SDLK_H:
        evnt = "h";
        break;
    case SDLK_I:
        evnt = "i";
        break;
    case SDLK_J:
        evnt = "j";
        break;
    case SDLK_K:
        evnt = "k";
        break;
    case SDLK_L:
        evnt = "l";
        break;
    case SDLK_M:
        evnt = "m";
        break;
    case SDLK_N:
        evnt = "n";
        break;
    case SDLK_O:
        evnt = "o";
        break;
    case SDLK_P:
        evnt = "p";
        break;
    case SDLK_Q:
        evnt = "q";
        break;
    case SDLK_R:
        evnt = "r";
        break;
    case SDLK_S:
        evnt = "s";
        break;
    case SDLK_T:
        evnt = "t";
        break;
    case SDLK_U:
        evnt = "u";
        break;
    case SDLK_V:
        evnt = "v";
        break;
    case SDLK_W:
        evnt = "w";
        break;
    case SDLK_X:
        evnt = "x";
        break;
    case SDLK_Y:
        evnt = "y";
        break;
    case SDLK_Z:
        evnt = "z";
        break;
    case SDLK_0:
        evnt = "0";
        break;
    case SDLK_1:
        evnt = "1";
        break;
    case SDLK_2:
        evnt = "2";
        break;
    case SDLK_3:
        evnt = "3";
        break;
    case SDLK_4:
        evnt = "4";
        break;
    case SDLK_5:
        evnt = "5";
        break;
    case SDLK_6:
        evnt = "6";
        break;
    case SDLK_7:
        evnt = "7";
        break;
    case SDLK_8:
        evnt = "8";
        break;
    case SDLK_9:
        evnt = "9";
        break;
    case SDLK_SLASH:
        evnt = "/";
        break;
    case SDLK_PERIOD:
        evnt = ".";
        break;
    case SDLK_MINUS:
        evnt = "-";
        break;
    case SDLK_COMMA:
        evnt = ",";
        break;
    case SDLK_SEMICOLON:
        evnt = ";";
        break;
    case SDLK_EQUALS:
        evnt = "=";
        break;
    case SDLK_APOSTROPHE:
        evnt = "'";
        break;
    case SDLK_LEFTBRACKET:
        evnt = "[";
        break;
    case SDLK_RIGHTBRACKET:
        evnt = "]";
        break;
    case SDLK_BACKSLASH:
        evnt = "\\";
        break;
    case SDLK_CAPSLOCK:
        evnt = "capslock";
        break;
    case SDLK_TAB:
        evnt = "tab";
        break;
    case SDLK_GRAVE:
        evnt = "`";
        break;
    case SDLK_UP:
        evnt = "arrow_up";
        break;
    case SDLK_DOWN:
        evnt = "arrow_down";
        break;
    case SDLK_LEFT:
        evnt = "arrow_left";
        break;
    case SDLK_RIGHT:
        evnt = "arrow_right";
        break;
    case SDLK_F1:
        evnt = "f1";
        break;
    case SDLK_F2:
        evnt = "f2";
        break;
    case SDLK_F3:
        evnt = "f3";
        break;
    case SDLK_F4:
        evnt = "f4";
        break;
    case SDLK_F5:
        evnt = "f5";
        break;
    case SDLK_F6:
        evnt = "f6";
        break;
    case SDLK_F7:
        evnt = "f7";
        break;
    case SDLK_F8:
        evnt = "f8";
        break;
    case SDLK_F9:
        evnt = "f9";
        break;
    case SDLK_F10:
        evnt = "f10";
        break;
    case SDLK_F11:
        evnt = "f11";
        break;
    case SDLK_F12:
        evnt = "f12";
        break;
    case SDLK_F13:
        evnt = "f13";
        break;
    case SDLK_F14:
        evnt = "f14";
        break;
    case SDLK_F15:
        evnt = "f15";
        break;
    case SDLK_LALT:
        evnt = "left_alt";
        break;
    case SDLK_RALT:
        evnt = "right_alt";
        break;
    case SDLK_RCTRL:
        evnt = "right_control";
        break;
    case SDLK_LCTRL:
        evnt = "left_control";
        break;
    case SDLK_RSHIFT:
        evnt = "right_shift";
        break;
    case SDLK_LSHIFT:
        evnt = "left_shift";
        break;
    case SDLK_INSERT:
        evnt = "insert";
        break;
    case SDLK_HOME:
        evnt = "home";
        break;
    case SDLK_DELETE:
        evnt = "delete";
        break;
    case SDLK_PAGEUP:
        evnt = "pageup";
        break;
    case SDLK_PAGEDOWN:
        evnt = "pagedown";
        break;
    case SDLK_SPACE:
        evnt = "space";
        break;
    case SDLK_BACKSPACE:
        evnt = "backspace";
        break;
    case SDLK_END:
        evnt = "end";
        break;
    default:
        break;
    }
    return evnt;
}

/**
 **/
bool Keyboard::handleEvent(const SDL_Event& event) {
    std::string keyName;
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
        keyName = keyEvent(event.key.key);
        // a key we have no name for cannot be bound to anything, and must not reach
        // KeyState either - it would be held under an empty name that nothing can ask for
        if (keyName.empty()) {
            return true;
        }
        if (state_.held(keyName) != pressed) {
            state_(keyName);
        }
        if (pressed) {
            dispatcher_->trigger<v3d::event::kind::KeyDown>(v3d::event::kind::KeyDown(keyName, context_));
        } else {
            dispatcher_->trigger<v3d::event::kind::KeyUp>(v3d::event::kind::KeyUp(keyName, context_));
        }
        break;
    default:
        return false;
    }

    // trigger an event source event so any mappers can propogate any mapped events.
    // the edge is carried as the event's state, not as its data - data is the binding's
    // parameter, and the two would otherwise overwrite each other.
    v3d::event::Event source(keyName, context_);
    source.type(v3d::event::Type::Source);
    source.state(pressed ? v3d::event::State::Pressed : v3d::event::State::Released);
    dispatcher_->trigger(source);

    return true;
}

};  // namespace v3d::input
