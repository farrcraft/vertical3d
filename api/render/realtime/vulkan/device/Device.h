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
 * a surface is needed for. Everything that draws works on a headless device. Everything that
 * presents, such as a Swapchain or a Presenter, needs a device that was given a surface.
 **/
class Device final {
 public:
    /**
     * The queue families a device has to provide to draw, and to present when there is a
     * surface. The same family often serves both roles.
     **/
    struct QueueFamilies {
        QueueFamilies() noexcept;

        /**
         * @param presenting whether a present family is one of the ones needed, which it is
         *        only for a device that was given a surface
         * @return whether the needed families were found
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
     *        found. Regions of larger blocks unless a consumer asks for one allocation per
     *        resource
     **/
    Device(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Instance>& instance,
        const boost::shared_ptr<Surface>& surface = nullptr,
        memory::Allocator::Kind allocations = memory::Allocator::Kind::Suballocated);

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

    /**
     * Whether a pipeline may draw triangles as their edges or their corners, with the polygon
     * modes VK_POLYGON_MODE_LINE and VK_POLYGON_MODE_POINT. Those need the fillModeNonSolid
     * feature, which the device turns on when the gpu offers it. A device without it is still
     * chosen, because nothing else needs it, so a pipeline that would draw that way draws filled.
     **/
    bool nonSolidFill() const noexcept;

    /**
     * Choose the extensions a logical device is created with.
     *
     * Public because it has no side effects and needs no device, so a test can check both
     * branches on a machine with no portability driver.
     *
     * @param presenting whether the device presents, which needs the swapchain extension
     * @param offered the names of the extensions the physical device advertises
     * @return the swapchain extension when presenting, and the portability subset when the
     *         device offers it, which a device that offers it must have enabled
     **/
    static std::vector<const char*> extensions(bool presenting, const std::vector<std::string>& offered);

 private:
    /**
     * Pick the first physical device that can draw, and present when there is a surface,
     * preferring a discrete gpu.
     * @throw std::runtime_error if none of them can
     **/
    void selectPhysical();

    /**
     * @return the graphics family a physical device offers, and the present family it offers
     *         for the surface when there is one
     **/
    QueueFamilies findFamilies(VkPhysicalDevice device) const;

    /**
     * @return the names of the extensions a physical device advertises
     **/
    static std::vector<std::string> offered(VkPhysicalDevice device);

    /**
     * @param presenting whether the swapchain extension is among the ones needed
     * @return the first extension the renderer needs that a physical device does not
     *         advertise, or empty when it advertises them all
     **/
    static std::string missingExtension(VkPhysicalDevice device, bool presenting);

    /**
     * @return the first of the 1.3 features the renderer draws with, and the separate depth
     *         and stencil layouts its depth barriers rely on, that a physical device lacks, or
     *         empty when it has them all
     **/
    static std::string missingFeature(VkPhysicalDevice device);

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
    bool nonSolidFill_;
    /**< created after the logical device. Everything it allocated is freed before it goes **/
    boost::shared_ptr<memory::Allocator> allocator_;
};

};  // namespace v3d::render::realtime::vulkan::device
