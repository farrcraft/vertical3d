/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loaded.h"

#include <api/log/Logger.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::internal {

/**
 **/
Loaded::Loaded(const boost::shared_ptr<v3d::log::Logger>& logger) : instance(logger, extensions()) {
}

/**
 **/
std::vector<const char*> Loaded::extensions() {
    uint32_t count = 0;
    const char* const* names = SDL_Vulkan_GetInstanceExtensions(&count);
    if (names == nullptr) {
        throw std::runtime_error("SDL could not name the Vulkan extensions a window needs: " + std::string(SDL_GetError()));
    }
    return std::vector<const char*>(names, names + count);
}

};  // namespace v3d::render::realtime::internal
