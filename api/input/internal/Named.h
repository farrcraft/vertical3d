/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <SDL3/SDL_keycode.h>

#include <string_view>

namespace v3d::input::internal {

/**
 * Every key this library has a name for, and the name. Read in both directions: an SDL key
 * to the name a binding and a text box use, and a name back to whether it is one.
 **/
struct Named {
    SDL_Keycode key;
    std::string_view name;
};

};  // namespace v3d::input::internal
