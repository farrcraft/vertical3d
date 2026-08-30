/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "Instance.h"
#include "Surface.h"

#include "../../../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

    /**
     * A logical device selected from the physical devices the instance can see,
     * together with the queues the renderer draws and presents with.
     **/
    class Device final {
     public:
        /**
         * The queue families a device has to provide before we can render to a window with it.
         * The same family often serves both roles.
         **/
        struct QueueFamilies {
            QueueFamilies() noexcept;

            /**
             * @return whether both of the families we need were found
             **/
            bool complete() const noexcept;

            uint32_t graphics;
            uint32_t present;
            bool hasGraphics;
            bool hasPresent;
        };

        /**
         * @param logger
         * @param instance the instance to select a physical device from
         * @param surface the surface the device has to be able to present to
         **/
        Device(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Instance>& instance, const boost::shared_ptr<Surface>& surface);

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
         * @return the queue families backing the device
         **/
        const QueueFamilies& families() const noexcept;

        /**
         * @return the queue draw commands are submitted to
         **/
        VkQueue graphicsQueue() const noexcept;

        /**
         * @return the queue finished images are presented on
         **/
        VkQueue presentQueue() const noexcept;

     private:
        /**
         * Pick the first physical device that can both render and present, preferring a discrete gpu.
         * @throw std::runtime_error if none of them can
         **/
        void selectPhysical();

        /**
         * @return the graphics and present families a physical device offers for our surface
         **/
        QueueFamilies findFamilies(VkPhysicalDevice device) const;

        /**
         * @return whether a physical device advertises every extension the renderer needs
         **/
        bool hasRequiredExtensions(VkPhysicalDevice device) const;

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
    };

};  // namespace v3d::render::realtime::vulkan
