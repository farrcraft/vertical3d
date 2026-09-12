/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
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

    /**
     * What the layer has reported since the instance was created, which is what a render
     * test asserts on rather than a picture - ADR-0007.
     *
     * Assert validating() alongside these: where the layer is not installed they stay zero
     * because nothing was watching, which reads exactly like a clean run.
     *
     * @return how many messages arrived at error severity
     **/
    uint32_t errors() const noexcept;

    /**
     * @return how many messages arrived at warning severity
     **/
    uint32_t warnings() const noexcept;

    /**
     * @return the first error reported, or empty when there has been none. Only the first is
     *         kept - a count is what a caller acts on, and every message goes to the logger
     *         anyway, so holding all of them for the life of an instance buys nothing
     **/
    const std::string& firstError() const noexcept;

 private:
    /**
     * The messenger's callback. Counts what arrives and puts it through the logger at a
     * severity matching its own.
     *
     * @param user the instance that created the messenger
     **/
    static VKAPI_ATTR VkBool32 VKAPI_CALL report(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user);

    /**
     * Check the requested extensions against the ones the loader advertises.
     * @throw std::runtime_error if any of them are unavailable
     **/
    static void requireExtensions(const std::vector<const char*>& extensions);

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
    uint32_t errors_;
    uint32_t warnings_;
    std::string firstError_;
};

};  // namespace v3d::render::realtime::vulkan::device
