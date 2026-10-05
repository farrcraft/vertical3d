/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/pipeline/Pipeline.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include "FrameUniforms.h"
#include "Timings.h"

namespace v3d::render::realtime::vulkan::frame {

/**
 * Turns a frame into commands.
 *
 * Recording belongs to the engine rather than to each draw, so this is the only place that
 * writes to a command buffer. It draws through dynamic rendering: there is no VkRenderPass
 * and no VkFramebuffer anywhere in the renderer, which relies on Vulkan 1.3.
 *
 * A pass is recorded in submission order, which 2D content needs, unless it asks to be
 * sorted - see Pass::sort. A sorted pass is recorded in sort key order, so items sharing a
 * pipeline and a material end up adjacent and need no binds between them.
 *
 * Nothing already bound is rebound: a pipeline and a descriptor set are bound only when
 * an item names a different one than the last item did, so a run of quads sharing a
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

        VkImage image;         /**< transitioned for drawing and then into finalLayout **/
        VkImageView view;      /**< the colour attachment the passes draw into **/
        VkExtent2D extent;     /**< the size of the image **/
        VkImage depthImage;    /**< the depth buffer, or null when there is none **/
        VkImageView depthView; /**< the attachment a pass that depth tests draws into **/
        bool sampledDepth;     /**< whether a later pass reads that depth image **/
        VkFormat format;       /**< the colour format, or undefined when it goes unchecked **/
        VkFormat depthFormat;  /**< the depth format, or undefined when it goes unchecked **/

        /**
         * What the frame leaves the image in.
         *
         * PRESENT_SRC by default, because a frame is usually drawn to be presented and that
         * is the layout the presentation engine reads. A frame with no chain under it is not
         * presented and cannot use it: the layout is only valid where VK_KHR_swapchain is
         * enabled, so using it on a headless device is a validation error. Such a frame names
         * the layout its next use needs instead, which is SHADER_READ_ONLY_OPTIMAL for one that
         * is captured or sampled afterwards.
         **/
        VkImageLayout finalLayout;
    };

    /**
     * Record a whole frame, including the layout transitions either side of it.
     * @param commands a command buffer that has already been begun
     * @param resources what the frame's draw items name by handle
     * @param uniforms where each pass's camera is written and bound from, or null for a
     *        frame whose pipelines declare nothing at set 0
     * @param timings where each pass is timed under its name, or null for none
     **/
    static void record(VkCommandBuffer commands, const Frame& frame, const Target& target, const pipeline::Resources& resources,
        FrameUniforms* uniforms = nullptr, Timings* timings = nullptr);

    /**
     * Whether a pass gives a pipeline everything it declares it needs per pass: a scene set
     * for a pipeline whose layout has a set 2, and a bias for one built with depth bias.
     *
     * Also whether the pipeline was built for what the pass draws into: its colour format,
     * and its depth format when the pass tests depth, wherever both sides state one. The
     * validation layer reports a mismatch only when it is enabled; this check names the pass
     * in every build.
     *
     * Checked whenever the recorder binds a pipeline. It needs no device, so it can be
     * tested on its own.
     *
     * @param into what the pass draws into, whose formats go unchecked where it leaves them
     *        undefined
     * @throw std::runtime_error naming the pass and what it lacks
     **/
    static void check(const Pass& pass, const pipeline::Pipeline& pipeline, const Target& into = Target());

 private:
    /**
     * What the last item recorded left bound, so the next one can skip rebinding it.
     **/
    struct Bound {
        Bound() noexcept;

        const pipeline::Pipeline* pipeline;
        VkDescriptorSet frameSet;
        VkDescriptorSet sceneSet;
        VkDescriptorSet set;
        VkBuffer vertexBuffer;
        VkDeviceSize vertexBufferOffset;
        VkBuffer indexBuffer;
        VkDeviceSize indexBufferOffset;
        VkRect2D area;      /**< all of what the pass draws into, which an unclipped item uses **/
        VkRect2D scissor;   /**< what is set now, so an unchanged clip costs nothing **/
        bool scissorSet;    /**< false until one is known, as after an escape hatch has run **/
        const Target* into; /**< what the pass draws into, which every pipeline it binds is checked against. Set before any item is recorded **/
    };

    /**
     * Bring a target into the layouts its passes draw into, before the first pass of the frame
     * that writes it. A target with no colour image has only its depth moved.
     *
     * @param depth whether any pass of the frame that writes the target uses its depth, which
     *        is not always the first one
     **/
    static void openTarget(VkCommandBuffer commands, bool depth, const Target& into);

    /**
     * Leave a target readable after the last pass of the frame that writes it: colour in
     * SHADER_READ_ONLY_OPTIMAL, and a sampled depth image in DEPTH_READ_ONLY_OPTIMAL.
     *
     * @param depth whether any pass of the frame that writes the target uses its depth, which
     *        is not always the last one
     **/
    static void closeTarget(VkCommandBuffer commands, bool depth, const Target& into);

    /**
     * @param frameSet what the pass binds at set 0, or null if it binds nothing there
     **/
    static void record(VkCommandBuffer commands, const Pass& pass, const Target& target, const pipeline::Resources& resources,
        VkDescriptorSet frameSet);

    /**
     * Bind what the item needs that is not bound already, and issue its draw.
     **/
    static void record(VkCommandBuffer commands, const Pass& pass, const DrawItem& item, const pipeline::Resources& resources,
        VkDescriptorSet frameSet, Bound* bound);

    /**
     * Cut the draw down to the item's scissor, or back to the whole pass when it names
     * none. The rectangle is clamped to the pass's own, so an item
     * clipped against a canvas larger than the image cannot name a region outside it.
     **/
    static void scissor(VkCommandBuffer commands, const DrawItem& item, Bound* bound);
};

};  // namespace v3d::render::realtime::vulkan::frame
