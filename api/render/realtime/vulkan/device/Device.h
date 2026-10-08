/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Instance.h"
#include "Surface.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::device {

/**
 * A logical device selected from the physical devices the instance can see,
 * together with the queues the renderer draws and presents with.
 *
 * A device given no surface is headless. It is selected on its graphics family alone, asks
 * for no swapchain extension, and has no present queue, because presenting is the only thing
 * a surface is needed for. Everything that draws works on one; everything that presents - a
 * Swapchain, a Presenter - needs a device that was given a surface.
 **/
class Device final {
 public:
    /**
     * The queue families a device has to provide before we can draw with it, and present
     * from it when there is a surface. The same family often serves both roles.
     **/
    struct QueueFamilies {
        QueueFamilies() noexcept;

        /**
         * @param presenting whether a present family is one of the ones needed, which it is
         *        only for a device that was given a surface
         * @return whether the families we need were found
         **/
        bool complete(bool presenting) const noexcept;

        uint32_t graphics;
        uint32_t present;
        bool hasGraphics;
        bool hasPresent;
        uint32_t timestampBits;  /**< how many bits of a timestamp the graphics family writes, 0 for none **/
    };

    /**
     * What the device is, for a diagnostics readout, in types that need no Vulkan include to
     * read. Filled once, when the device is selected.
     **/
    struct Description final {
        std::string name;           /**< as the driver reports it **/
        uint32_t apiMajor = 0;      /**< the Vulkan version the device supports **/
        uint32_t apiMinor = 0;
        uint32_t apiPatch = 0;
        uint32_t vendor = 0;        /**< the PCI vendor id **/
        uint32_t driver = 0;        /**< the driver version, encoded as its vendor encodes it **/
    };

    /**
     * @param logger
     * @param instance the instance to select a physical device from
     * @param surface the surface the device has to be able to present to, or null for a
     *        headless device that only draws
     * @param allocations how the memory behind every buffer and image on this device is
     *        found. One allocation per resource unless a consumer asks otherwise, as an
     *        application with per-frame resources should
     **/
    Device(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Instance>& instance,
        const boost::shared_ptr<Surface>& surface = nullptr,
        memory::Allocator::Kind allocations = memory::Allocator::Kind::Direct);

    /**
     **/
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    /**
     * @return the underlying logical device handle
     **/
    VkDevice handle() const noexcept;

    /**
     * @return the physical device the logical one was created from
     **/
    VkPhysicalDevice physical() const noexcept;

    /**
     * @return what gives every buffer and image on this device the memory it lives in
     **/
    memory::Allocator& allocator() const noexcept;

    /**
     * @return the surface the device was selected to present to, or null on a headless one
     **/
    boost::shared_ptr<Surface> surface() const noexcept;

    /**
     * @return whether the device was given a surface, and so has a present queue
     **/
    bool presenting() const noexcept;

    /**
     * @return the queue families backing the device
     **/
    const QueueFamilies& families() const noexcept;

    /**
     * @return the queue draw commands are submitted to
     **/
    VkQueue graphicsQueue() const noexcept;

    /**
     * @return how many nanoseconds one tick of a timestamp is
     **/
    float timestampPeriod() const noexcept;

    /**
     * @return the queue finished images are presented on, or null on a headless device
     **/
    VkQueue presentQueue() const noexcept;

    /**
     * @return the device's name, Vulkan version, vendor and driver
     **/
    const Description& description() const noexcept;

 private:
    /**
     * Pick the first physical device that can draw, and present when there is a surface,
     * preferring a discrete gpu.
     * @throw std::runtime_error if none of them can
     **/
    void selectPhysical();

    /**
     * @return the graphics family a physical device offers, and the present family it offers
     *         for our surface when there is one
     **/
    QueueFamilies findFamilies(VkPhysicalDevice device) const;

    /**
     * @param presenting whether the swapchain extension is among the ones needed
     * @return whether a physical device advertises every extension the renderer needs
     **/
    static bool hasRequiredExtensions(VkPhysicalDevice device, bool presenting);

    /**
     * @return whether a physical device offers the 1.3 features the renderer draws with, and
     *         the separate depth and stencil layouts its depth barriers rely on
     **/
    static bool hasRequiredFeatures(VkPhysicalDevice device);

    /**
     **/
    void createLogical();

    boost::shared_ptr<Instance> instance_;
    boost::shared_ptr<Surface> surface_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    VkPhysicalDevice physical_;
    VkDevice device_;
    QueueFamilies families_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    float timestampPeriod_;
    Description description_;
    /**< built once the logical device exists, and outlived by nothing it allocated for **/
    boost::shared_ptr<memory::Allocator> allocator_;
};

};  // namespace v3d::render::realtime::vulkan::device
