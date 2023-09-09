/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keyboard.h"

#include <string>

#include "../event/KeyDown.h"
#include "../event/KeyUp.h"
#include "../event/Source.h"

namespace v3d::input {

    /**
     * Map a SDL key symbol to a string representation
     **/
    std::string keyEvent(const SDL_Keysym* keysym) {
        std::string evnt;

        switch (keysym->sym) {
        case SDLK_ESCAPE:
            evnt = "escape";
            break;
        case SDLK_RETURN:
            evnt = "return";
            break;
        case SDLK_a:
            evnt = "a";
            break;
        case SDLK_b:
            evnt = "b";
            break;
        case SDLK_c:
            evnt = "c";
            break;
        case SDLK_d:
            evnt = "d";
            break;
        case SDLK_e:
            evnt = "e";
            break;
        case SDLK_f:
            evnt = "f";
            break;
        case SDLK_g:
            evnt = "g";
            break;
        case SDLK_h:
            evnt = "h";
            break;
        case SDLK_i:
            evnt = "i";
            break;
        case SDLK_j:
            evnt = "j";
            break;
        case SDLK_k:
            evnt = "k";
            break;
        case SDLK_l:
            evnt = "l";
            break;
        case SDLK_m:
            evnt = "m";
            break;
        case SDLK_n:
            evnt = "n";
            break;
        case SDLK_o:
            evnt = "o";
            break;
        case SDLK_p:
            evnt = "p";
            break;
        case SDLK_q:
            evnt = "q";
            break;
        case SDLK_r:
            evnt = "r";
            break;
        case SDLK_s:
            evnt = "s";
            break;
        case SDLK_t:
            evnt = "t";
            break;
        case SDLK_u:
            evnt = "u";
            break;
        case SDLK_v:
            evnt = "v";
            break;
        case SDLK_w:
            evnt = "w";
            break;
        case SDLK_x:
            evnt = "x";
            break;
        case SDLK_y:
            evnt = "y";
            break;
        case SDLK_z:
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
        case SDLK_EXCLAIM:
            evnt = "!";
            break;
        case SDLK_MINUS:
            evnt = "-";
            break;
        case SDLK_COMMA:
            evnt = ",";
            break;
        case SDLK_PLUS:
            evnt = "+";
            break;
        case SDLK_ASTERISK:
            evnt = "*";
            break;
        case SDLK_SEMICOLON:
            evnt = ";";
            break;
        case SDLK_CAPSLOCK:
            evnt = "capslock";
            break;
        case SDLK_TAB:
            evnt = "tab";
            break;
        case SDLK_BACKQUOTE:
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
        case SDL_KEYDOWN:
            keyName = keyEvent(&event.key.keysym);
            if (!state_.pressed(keyName)) {
                state_(keyName);
            }
            dispatcher_->trigger<v3d::event::KeyDown>(v3d::event::KeyDown(keyName, context_));
            break;
        case SDL_KEYUP:
            keyName = keyEvent(&event.key.keysym);
            if (!state_.pressed(keyName)) {
                state_(keyName);
            }
            pressed = false;
            dispatcher_->trigger<v3d::event::KeyUp>(v3d::event::KeyUp(keyName, context_));
            break;
        default:
            return false;
        }

        // trigger an event source event so any mappers can propogate any mapped events
        v3d::event::Source source(keyName, context_);
        v3d::event::EventData keyState = pressed;
        source.data(keyState);
        dispatcher_->trigger(source);

        return true;
    }

};  // namespace v3d::input
