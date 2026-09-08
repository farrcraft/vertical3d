/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "FrameUniforms.h"
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
 * A pass is recorded in submission order unless it asks to be sorted, which is what 2D
 * content needs - see Pass::sort. A sorted pass is walked in sort key order, so items
 * sharing a pipeline and a material end up adjacent and the binds between them fall away.
 *
 * Nothing already bound is rebound: a pipeline and a descriptor set are bound only when
 * an item asks for a different one than the last item did, so a run of quads sharing a
 * texture costs one bind between them.
 **/
class Recorder final {
 public:
    /**
     * What a frame is being recorded into. The swapchain image the presenter acquired,
     * and the depth buffer the context keeps beside it.
     **/
    struct Target {
        Target() noexcept;

        VkImage image;         /**< transitioned for drawing and then for presenting **/
        VkImageView view;      /**< the colour attachment the passes draw into **/
        VkExtent2D extent;     /**< the size of the image **/
        VkImage depthImage;    /**< the depth buffer, or null when there is none **/
        VkImageView depthView; /**< the attachment a pass that depth tests draws into **/
        bool sampledDepth;     /**< whether a later pass reads that depth image **/
    };

    /**
     * Record a whole frame, including the layout transitions either side of it.
     * @param commands a command buffer that has already been begun
     * @param resources what the frame's draw items name by handle
     * @param uniforms where each pass's camera is written and bound from, or null for a
     *        frame whose pipelines declare nothing at set 0
     **/
    void record(VkCommandBuffer commands, const Frame& frame, const Target& target, const Resources& resources,
        FrameUniforms* uniforms = nullptr) const;

 private:
    /**
     * What the last item recorded left bound, so the next one can skip rebinding it.
     **/
    struct Bound {
        Bound() noexcept;

        const Pipeline* pipeline;
        VkDescriptorSet frameSet;
        VkDescriptorSet set;
        VkBuffer vertexBuffer;
        VkDeviceSize vertexBufferOffset;
        VkBuffer indexBuffer;
        VkDeviceSize indexBufferOffset;
        VkRect2D area;      /**< the whole of what the pass draws into, which an unclipped item wants **/
        VkRect2D scissor;   /**< what is set now, so an unchanged clip costs nothing **/
        bool scissorSet;    /**< false until one is known, which is what an escape hatch leaves behind **/
    };

    /**
     * Move the colour image between layouts with a synchronization2 barrier.
     **/
    static void transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to);

    /**
     * Bring the depth image into the layout a pass attaches it in. The contents are
     * discarded, which is why the first pass to use it in a frame has to clear.
     **/
    static void transitionDepth(VkCommandBuffer commands, VkImage image);

    /**
     * Leave a sampled depth image where a descriptor set can read it.
     *
     * DEPTH_READ_ONLY_OPTIMAL rather than SHADER_READ_ONLY_OPTIMAL: it is the layout a
     * depth aspect is both sampled and tested in, and it is what makes the promise a pass
     * sampling this depends on - that nothing writes the image while it is being read. A
     * pass that both samples a target's depth and draws into it is the hazard that
     * forbids rather than detects.
     **/
    static void transitionDepthForReading(VkCommandBuffer commands, VkImage image);

    /**
     * @param frameSet what the pass binds at set 0, or null if it binds nothing there
     **/
    static void record(VkCommandBuffer commands, const Pass& pass, const Target& target, const Resources& resources,
        VkDescriptorSet frameSet);

    /**
     * Bind what the item needs that is not bound already, and issue its draw.
     **/
    static void record(VkCommandBuffer commands, const DrawItem& item, const Resources& resources, VkDescriptorSet frameSet, Bound* bound);

    /**
     * Cut the draw down to what the item asks for, or back to the whole pass when it asks
     * for nothing, per ADR-0037. The rectangle is clamped to the pass's own, so an item
     * clipped against a canvas larger than the image cannot name a region outside it.
     **/
    static void scissor(VkCommandBuffer commands, const DrawItem& item, Bound* bound);
};

};  // namespace v3d::render::realtime::vulkan
