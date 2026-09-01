/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "Resources.h"

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
     * Items are recorded in submission order, which is the order 2D content has to be drawn
     * in - the only pass anything draws into so far is painter ordered. Sorting on the draw
     * item's key is what a depth tested scene pass will want, and belongs here.
     *
     * Nothing already bound is rebound: a pipeline and a descriptor set are bound only when
     * an item asks for a different one than the last item did, so a run of quads sharing a
     * texture costs one bind between them.
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
         * @param resources what the frame's draw items name by handle
         **/
        void record(VkCommandBuffer commands, const Frame& frame, const Target& target, const Resources& resources) const;

     private:
        /**
         * What the last item recorded left bound, so the next one can skip rebinding it.
         **/
        struct Bound {
            Bound() noexcept;

            const Pipeline* pipeline;
            VkDescriptorSet set;
            VkBuffer vertexBuffer;
            VkDeviceSize vertexBufferOffset;
            VkBuffer indexBuffer;
            VkDeviceSize indexBufferOffset;
        };

        /**
         * Move an image between layouts with a synchronization2 barrier.
         **/
        static void transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to);

        /**
         **/
        static void record(VkCommandBuffer commands, const Pass& pass, const Target& target, const Resources& resources);

        /**
         * Bind what the item needs that is not bound already, and issue its draw.
         **/
        static void record(VkCommandBuffer commands, const DrawItem& item, const Resources& resources, Bound* bound);
    };

};  // namespace v3d::render::realtime::vulkan
