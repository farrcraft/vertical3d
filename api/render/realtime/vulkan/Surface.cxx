/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Surface.h"

#include <SDL3/SDL_vulkan.h>

#include <sstream>
#include <stdexcept>
#include <string>

namespace v3d::render::realtime::vulkan {

/**
 **/
Surface::Surface(const boost::shared_ptr<Instance>& instance, SDL_Window* window) :
    instance_(instance),
    surface_(VK_NULL_HANDLE) {
    if (!SDL_Vulkan_CreateSurface(window, instance_->handle(), nullptr, &surface_)) {
        std::stringstream msg;
        msg << "Unable to create vulkan surface - " << SDL_GetError();
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
Surface::~Surface() {
    if (surface_ != VK_NULL_HANDLE) {
        SDL_Vulkan_DestroySurface(instance_->handle(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

/**
 **/
VkSurfaceKHR Surface::handle() const noexcept {
    return surface_;
}

};  // namespace v3d::render::realtime::vulkan
