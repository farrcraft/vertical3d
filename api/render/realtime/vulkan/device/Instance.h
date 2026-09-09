/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <vulkan/vulkan.h>

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::device {

/**
 * The application's connection to the vulkan loader.
 * Owns the underlying VkInstance for the lifetime of the object.
 *
 * When the Khronos validation layer is installed it is enabled, along with a messenger that
 * puts what it says through the logger. The messenger is not optional: a layer with
 * nowhere to report to is silent, which is indistinguishable from a clean run.
 **/
class Instance final {
 public:
    /**
     * @param logger
     * @param extensions the instance extensions required by the windowing system
     **/
    Instance(const boost::shared_ptr<v3d::log::Logger>& logger, const std::vector<const char*>& extensions);

    /**
     **/
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    /**
     * @return the underlying vulkan instance handle
     **/
    VkInstance handle() const noexcept;

    /**
     * @return whether the validation layer was found and enabled
     **/
    bool validating() const noexcept;

 private:
    /**
     * Check the requested extensions against the ones the loader advertises.
     * @throw std::runtime_error if any of them are unavailable
     **/
    void requireExtensions(const std::vector<const char*>& extensions) const;

    /**
     * @return whether the loader advertises an instance extension
     **/
    static bool hasExtension(const char* extension);

    /**
     * @return whether the loader advertises the Khronos validation layer
     **/
    static bool hasValidationLayer();

    /**
     * Route the validation layer's messages to the logger. Does nothing when the layer
     * is not installed, or when the debug utils extension is not there to carry them.
     **/
    void createMessenger();

    VkInstance instance_;
    VkDebugUtilsMessengerEXT messenger_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    bool validating_;
};

};  // namespace v3d::render::realtime::vulkan::device
