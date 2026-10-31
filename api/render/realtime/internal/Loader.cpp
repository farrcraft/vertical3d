/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loader.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include <stdexcept>
#include <string>

namespace v3d::render::realtime::internal {

/**
 **/
Loader::Loader() {
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        throw std::runtime_error("The Vulkan loader could not be loaded: " + std::string(SDL_GetError()));
    }
}

/**
 **/
Loader::~Loader() {
    SDL_Vulkan_UnloadLibrary();
}

};  // namespace v3d::render::realtime::internal
