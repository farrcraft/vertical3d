/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Resources.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Timings.h>
#include <api/render/realtime/vulkan/pipeline/Pipeline.h>

#include <vulkan/vulkan.h>

namespace v3d::render::realtime {

/**
 * Turns a frame into commands.
 *
 * Recorder records every pass of a frame. A draw item describes its draw in fields and does
 * not record commands itself, except through DrawItem::record. Recorder draws through dynamic
 * rendering, so the renderer has no VkRenderPass and no VkFramebuffer. Dynamic rendering
 * requires Vulkan 1.3.
 *
 * A pass records its items in submission order, which 2D content needs, unless Pass::sort is
 * enabled. A sorted pass records in sort key order, so items that share a pipeline and a
 * material are adjacent and need no binds between them.
 *
 * Nothing already bound is rebound. A pipeline and a descriptor set are bound only when
 * an item names a different one than the last item did, so a run of quads sharing a
 * texture costs one bind between them.
 **/
class Recorder final {
 public:
    /**
     * What a frame is recorded into: usually the swapchain image that Presenter acquired, and
     * the depth buffer that DeviceContext keeps beside it.
     **/
    struct Target {
        Target() noexcept;

        VkImage image;         /**< transitioned for drawing and then into finalLayout **/
        VkImageView view;      /**< the colour attachment the passes draw into **/
        VkExtent2D extent;     /**< the size of the image **/
        VkImage depthImage;    /**< the depth buffer, or null when there is none **/
        VkImageView depthView; /**< the attachment a pass that depth tests draws into **/
        bool sampledDepth;     /**< whether a later pass reads that depth image **/
        VkFormat format;       /**< the colour format, or undefined to skip the format check **/
        VkFormat depthFormat;  /**< the depth format, or undefined to skip the format check **/

        /**
         * What the frame leaves the image in.
         *
         * PRESENT_SRC by default, because most frames are presented and the presentation
         * engine reads that layout. A frame drawn without a swapchain cannot use PRESENT_SRC.
         * The layout is valid only where VK_KHR_swapchain is enabled, and using it on a
         * headless device is a validation error. Such a frame names the layout its next use
         * needs, such as SHADER_READ_ONLY_OPTIMAL for an image captured or sampled afterwards.
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
    static void record(VkCommandBuffer commands, const Frame& frame, const Target& target, const Resources& resources,
        vulkan::frame::FrameUniforms* uniforms = nullptr, vulkan::frame::Timings* timings = nullptr);

    /**
     * Whether a pass gives a pipeline everything it declares it needs per pass: a scene set
     * for a pipeline whose layout has a set 2, and a bias for one built with depth bias.
     *
     * Also whether the pipeline was built for what the pass draws into: its colour format,
     * and its depth format when the pass tests depth, wherever both sides state one. The
     * validation layer reports a mismatch only when it is enabled; this check names the pass
     * in every build.
     *
     * record() calls it whenever it binds a pipeline. It needs no device, so it can be tested
     * on its own.
     *
     * @param into what the pass draws into, whose formats go unchecked where it leaves them
     *        undefined
     * @throw std::runtime_error naming the pass and what it lacks
     **/
    static void check(const Pass& pass, const vulkan::pipeline::Pipeline& pipeline, const Target& into = Target());

 private:
    /**
     * What the last item recorded left bound, so the next one can skip rebinding it.
     **/
    struct Bound {
        Bound() noexcept;

        const vulkan::pipeline::Pipeline* pipeline;
        VkDescriptorSet frameSet;
        VkDescriptorSet sceneSet;
        VkDescriptorSet set;
        VkBuffer vertexBuffer;
        VkDeviceSize vertexBufferOffset;
        VkBuffer indexBuffer;
        VkDeviceSize indexBufferOffset;
        VkRect2D area;      /**< the whole area the pass draws into, used by an item with no clip **/
        VkRect2D scissor;   /**< the scissor set now, so an unchanged clip costs nothing **/
        bool scissorSet;    /**< false while the scissor is unknown, as after DrawItem::record has run **/
        const Target* into; /**< what the pass draws into. Every pipeline it binds is checked against it. Set before any item is recorded **/
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
    static void record(VkCommandBuffer commands, const Pass& pass, const Target& target, const Resources& resources,
        VkDescriptorSet frameSet);

    /**
     * Bind what the item needs that is not bound already, and issue its draw.
     **/
    static void record(VkCommandBuffer commands, const Pass& pass, const DrawItem& item, const Resources& resources,
        VkDescriptorSet frameSet, Bound* bound);

    /**
     * Set the scissor to the item's clip, or to the whole pass area when it has none. The
     * rectangle is clamped to the pass area, so an item clipped against a canvas larger than
     * the image cannot name a region outside it.
     **/
    static void scissor(VkCommandBuffer commands, const DrawItem& item, Bound* bound);

    /**
     * Set the dynamic state a pipeline that was just bound declares and takes from the pass:
     * its depth bias, and whether it writes depth.
     *
     * @pre check() has passed, so a biased pipeline's pass names a bias
     **/
    static void dynamics(VkCommandBuffer commands, const Pass& pass, const vulkan::pipeline::Pipeline& pipeline);
};

};  // namespace v3d::render::realtime
