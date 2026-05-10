/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mouse.h"

namespace v3d::input {

    /**
     **/
    bool Mouse::handleEvent(const SDL_Event& event) {
        switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_UP:
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            break;
        case SDL_EVENT_MOUSE_MOTION:
            break;
        default:
            return false;
        }
        return true;
    }

};  // namespace v3d::input
