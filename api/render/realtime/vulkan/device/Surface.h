/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "Instance.h"

#include <SDL3/SDL.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::device {

/**
 * The vulkan presentation surface backing an SDL window.
 * Keeps the instance it was created from alive for its own lifetime.
 **/
class Surface final {
 public:
    /**
     * @param instance the instance the surface belongs to
     * @param window an SDL window created with the SDL_WINDOW_VULKAN flag
     **/
    Surface(const boost::shared_ptr<Instance>& instance, SDL_Window* window);

    /**
     **/
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    /**
     * @return the underlying vulkan surface handle
     **/
    VkSurfaceKHR handle() const noexcept;

 private:
    boost::shared_ptr<Instance> instance_;
    VkSurfaceKHR surface_;
};

};  // namespace v3d::render::realtime::vulkan::device
