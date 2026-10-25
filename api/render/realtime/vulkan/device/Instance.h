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
 *
 * A loader hides a portability driver, one that implements vulkan over another API as MoltenVK
 * does, from an instance that has not asked for one. The instance asks whenever the loader
 * offers the extension that does it, so such a driver is found. On a loader without it nothing
 * changes.
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
     * What an instance is created with.
     **/
    struct Enabling final {
        std::vector<const char*> extensions;
        VkInstanceCreateFlags flags = 0;
        bool portability = false;  /**< whether portability drivers are enumerated **/
    };

    /**
     * Choose the extensions and flags an instance is created with.
     *
     * Public because it has no side effects and needs no loader, so a test can check both
     * branches on a machine with no portability driver.
     *
     * @param requested the extensions the windowing system requires, which are always enabled
     * @param offered the names of the instance extensions the loader lists
     * @param validating whether the validation layer is enabled, which needs debug utils
     * @return the requested extensions, debug utils when validating, and portability
     *         enumeration with its flag when the loader offers it
     **/
    static Enabling enable(const std::vector<const char*>& requested, const std::vector<std::string>& offered, bool validating);

    /**
     * @return whether the instance enumerates portability drivers
     **/
    bool portability() const noexcept;

    /**
     * @return the underlying vulkan instance handle
     **/
    VkInstance handle() const noexcept;

    /**
     * @return whether the validation layer was found and enabled
     **/
    bool validating() const noexcept;

    /**
     * What the layer has reported since the instance was created. A render test asserts on
     * these.
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
     *         kept: a caller acts on the count, and every message goes to the logger anyway
     **/
    const std::string& firstError() const noexcept;

    /**
     * @return the first warning reported, or empty when there has been none
     **/
    const std::string& firstWarning() const noexcept;

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
     * @return the names of the instance extensions the loader advertises
     **/
    static std::vector<std::string> offered();

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
    bool portability_;
    uint32_t errors_;
    uint32_t warnings_;
    std::string firstError_;
    std::string firstWarning_;
};

};  // namespace v3d::render::realtime::vulkan::device
