/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "../Frame.h"
#include "../Pass.h"

namespace v3d::render::realtime::vulkan {

    /**
     * Turns a frame into commands.
     *
     * Recording is the engine's job rather than an operation's, per ADR-0004, so this is the
     * one place that touches a command buffer. It draws through dynamic rendering - there is
     * no VkRenderPass and no VkFramebuffer anywhere in the renderer, per ADR-0002.
     *
     * Items are recorded in submission order for now. Sorting on the draw item's key and
     * merging adjacent items that share a pipeline and material both belong here, and
     * neither can be written until there are pipelines and materials to compare.
     **/
    class Recorder final {
     public:
        /**
         * What a frame is being recorded into. The swapchain image the presenter acquired,
         * so far the only target there is.
         **/
        struct Target {
            Target() noexcept;

            VkImage image;      /**< transitioned for drawing and then for presenting **/
            VkImageView view;   /**< the colour attachment the passes draw into **/
            VkExtent2D extent;  /**< the size of the image **/
        };

        /**
         * Record a whole frame, including the layout transitions either side of it.
         * @param commands a command buffer that has already been begun
         **/
        void record(VkCommandBuffer commands, const Frame& frame, const Target& target) const;

     private:
        /**
         * Move an image between layouts with a synchronization2 barrier.
         **/
        static void transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to);

        /**
         **/
        static void record(VkCommandBuffer commands, const Pass& pass, const Target& target);
    };

};  // namespace v3d::render::realtime::vulkan
