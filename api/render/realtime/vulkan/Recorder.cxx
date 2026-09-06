/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Recorder.h"

#include <cstddef>
#include <vector>

#include "RenderTarget.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

namespace {

/**
 * Whether the pass at an index is the first of the frame to draw into the target it
 * names, or the last.
 *
 * A target is brought into the layout a pass attaches it in once, before the first pass
 * that writes it, and left readable after the last one - so two passes drawing into one
 * target cost one pair of barriers rather than two. Scanned rather than tallied because a
 * frame has a handful of passes, and a walk is easier to be sure of than a map that has
 * to be cleared every frame.
 **/
bool firstWrite(const std::vector<boost::shared_ptr<Pass>>& passes, std::size_t index) {
    for (std::size_t before = 0; before < index; ++before) {
        if (passes[before]->target() == passes[index]->target()) {
            return false;
        }
    }
    return true;
}

bool lastWrite(const std::vector<boost::shared_ptr<Pass>>& passes, std::size_t index) {
    for (std::size_t after = index + 1; after < passes.size(); ++after) {
        if (passes[after]->target() == passes[index]->target()) {
            return false;
        }
    }
    return true;
}

/**
 * Where a pass actually draws: a target of its own, or what the frame was given.
 **/
Recorder::Target resolve(const Pass& pass, const Recorder::Target& frame) {
    const boost::shared_ptr<RenderTarget>& offscreen = pass.target();
    if (!offscreen) {
        return frame;
    }

    Recorder::Target into;
    into.image = offscreen->image();
    into.view = offscreen->view();
    into.extent = offscreen->extent();
    into.depthImage = offscreen->depthImage();
    into.depthView = offscreen->depthView();
    return into;
}

/**
 * Write the pass's camera into the frame's uniform buffer and return the set it is bound
 * from, or null for a frame whose pipelines declare nothing at set 0.
 *
 * A viewport of zero means the whole of what is being drawn into, which is the target's
 * extent rather than the frame's - a pass into a smaller target sees that target's size.
 **/
VkDescriptorSet writeCamera(FrameUniforms* uniforms, const Pass& pass, const Recorder::Target& into) {
    if (uniforms == nullptr) {
        return VK_NULL_HANDLE;
    }

    FrameUniforms::Camera camera;
    camera.view = pass.view();
    camera.projection = pass.projection();
    camera.viewProjection = camera.projection * camera.view;
    camera.viewport.x = pass.viewport().x;
    camera.viewport.y = pass.viewport().y;
    camera.viewport.z = pass.viewport().z > 0.0f ? pass.viewport().z : static_cast<float>(into.extent.width);
    camera.viewport.w = pass.viewport().w > 0.0f ? pass.viewport().w : static_cast<float>(into.extent.height);
    return uniforms->write(camera);
}

};  // namespace

/**
 **/
Recorder::Target::Target() noexcept :
image(VK_NULL_HANDLE),
view(VK_NULL_HANDLE),
extent{0, 0},
depthImage(VK_NULL_HANDLE),
depthView(VK_NULL_HANDLE) {
}

/**
 **/
Recorder::Bound::Bound() noexcept :
pipeline(nullptr),
frameSet(VK_NULL_HANDLE),
set(VK_NULL_HANDLE),
vertexBuffer(VK_NULL_HANDLE),
vertexBufferOffset(0),
indexBuffer(VK_NULL_HANDLE),
indexBufferOffset(0) {
}

/**
 **/
void Recorder::record(VkCommandBuffer commands, const Frame& frame, const Target& target, const Resources& resources,
    FrameUniforms* uniforms) const {
    // the acquired image comes back in whatever layout it was left in, and nothing in the
    // frame reads it, so undefined is the honest source layout and the cheapest one
    transition(commands, target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // the context's depth buffer is only wanted by a pass drawing into the swapchain image -
    // a pass with a target of its own attaches that target's, at that target's size
    bool depth = false;
    for (const boost::shared_ptr<Pass>& pass : frame.passes()) {
        if (pass->depth() && !pass->target()) {
            depth = true;
            break;
        }
    }
    if (depth && target.depthImage != VK_NULL_HANDLE) {
        transitionDepth(commands, target.depthImage);
    }

    const std::vector<boost::shared_ptr<Pass>>& passes = frame.passes();
    for (std::size_t index = 0; index < passes.size(); ++index) {
        const boost::shared_ptr<Pass>& pass = passes[index];

        const Target into = resolve(*pass, target);
        const bool offscreen = static_cast<bool>(pass->target());

        if (offscreen && firstWrite(passes, index)) {
            // undefined as the source layout: a target carries nothing from one frame to the
            // next, the same way the swapchain image and the depth buffer do not. The
            // barrier still orders this frame's writes after the reads the previous frame
            // made of the same image
            transition(commands, into.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            if (pass->depth() && into.depthImage != VK_NULL_HANDLE) {
                transitionDepth(commands, into.depthImage);
            }
        }

        record(commands, *pass, into, resources, writeCamera(uniforms, *pass, into));

        // what a target is for: every pass after the last one that wrote it can sample it
        if (offscreen && lastWrite(passes, index)) {
            transition(commands, into.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    transition(commands, target.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

/**
 **/
void Recorder::record(VkCommandBuffer commands, const Pass& pass, const Target& target, const Resources& resources,
    VkDescriptorSet frameSet) {
    VkRenderingAttachmentInfo colour{};
    colour.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colour.imageView = target.view;
    colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // a pass that does not clear draws over what the pass before it left in the image
    colour.loadOp = pass.clears() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour.clearValue.color.float32[0] = pass.clearColour().r;
    colour.clearValue.color.float32[1] = pass.clearColour().g;
    colour.clearValue.color.float32[2] = pass.clearColour().b;
    colour.clearValue.color.float32[3] = pass.clearColour().a;

    const bool depth = pass.depth() && target.depthView != VK_NULL_HANDLE;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = target.depthView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    // depth follows colour: a pass that starts the image over starts the depth over too,
    // and one drawing on top of another keeps what that one wrote
    depthAttachment.loadOp = pass.clears() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // far is one - the projections in api/type are not reversed depth
    depthAttachment.clearValue.depthStencil.depth = 1.0f;

    // the region a pass draws into, which is the whole image until something asks for less
    VkRect2D area{};
    area.offset.x = static_cast<int32_t>(pass.viewport().x);
    area.offset.y = static_cast<int32_t>(pass.viewport().y);
    area.extent.width = pass.viewport().z > 0.0f ? static_cast<uint32_t>(pass.viewport().z) : target.extent.width;
    area.extent.height = pass.viewport().w > 0.0f ? static_cast<uint32_t>(pass.viewport().w) : target.extent.height;

    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea = area;
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &colour;
    rendering.pDepthAttachment = depth ? &depthAttachment : nullptr;

    vkCmdBeginRendering(commands, &rendering);

    // viewport and scissor are dynamic state everywhere, so a window resize costs no pipelines
    VkViewport viewport{};
    viewport.x = static_cast<float>(area.offset.x);
    viewport.y = static_cast<float>(area.offset.y);
    viewport.width = static_cast<float>(area.extent.width);
    viewport.height = static_cast<float>(area.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commands, 0, 1, &viewport);
    vkCmdSetScissor(commands, 0, 1, &area);

    // the pass decides whether that is submission order or sort key order
    std::vector<const DrawItem*> ordered;
    pass.ordered(&ordered);

    Bound bound;
    for (const DrawItem* item : ordered) {
        record(commands, *item, resources, frameSet, &bound);
    }

    vkCmdEndRendering(commands);
}

/**
 **/
void Recorder::record(VkCommandBuffer commands, const DrawItem& item, const Resources& resources, VkDescriptorSet frameSet, Bound* bound) {
    // the escape hatch of ADR-0004, for work the item's fields cannot describe. It
    // records whatever it likes, so nothing about what is bound survives it
    if (item.record) {
        item.record(commands);
        *bound = Bound();
        return;
    }

    const Pipeline* pipeline = resources.pipeline(item.pipeline);
    if (pipeline == nullptr || pipeline->pipeline == VK_NULL_HANDLE) {
        return;
    }

    if (pipeline != bound->pipeline) {
        vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
        bound->pipeline = pipeline;
        // a different layout invalidates what was bound against the old one
        bound->frameSet = VK_NULL_HANDLE;
        bound->set = VK_NULL_HANDLE;
    }

    if (frameSet != VK_NULL_HANDLE && frameSet != bound->frameSet) {
        // set 0 is the per frame frequency of ADR-0008 - the camera the whole pass draws
        // through, which is why it is bound here and never per item
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &frameSet, 0, nullptr);
        bound->frameSet = frameSet;
    }

    if (item.pushSize > 0 && pipeline->pushStages != 0) {
        vkCmdPushConstants(commands, pipeline->layout, pipeline->pushStages, 0, item.pushSize, item.push.data());
    }

    const Material* material = resources.material(item.material);
    if (material != nullptr && material->set != VK_NULL_HANDLE && material->set != bound->set) {
        // set 1 is the per material frequency of the same convention
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 1, 1, &material->set, 0, nullptr);
        bound->set = material->set;
    }

    if (item.vertexBuffer != VK_NULL_HANDLE &&
        (item.vertexBuffer != bound->vertexBuffer || item.vertexBufferOffset != bound->vertexBufferOffset)) {
        vkCmdBindVertexBuffers(commands, 0, 1, &item.vertexBuffer, &item.vertexBufferOffset);
        bound->vertexBuffer = item.vertexBuffer;
        bound->vertexBufferOffset = item.vertexBufferOffset;
    }

    if (item.indices > 0) {
        if (item.indexBuffer == VK_NULL_HANDLE) {
            return;
        }
        if (item.indexBuffer != bound->indexBuffer || item.indexBufferOffset != bound->indexBufferOffset) {
            vkCmdBindIndexBuffer(commands, item.indexBuffer, item.indexBufferOffset, item.indexType);
            bound->indexBuffer = item.indexBuffer;
            bound->indexBufferOffset = item.indexBufferOffset;
        }
        vkCmdDrawIndexed(commands, item.indices, item.instances, item.firstIndex, static_cast<int32_t>(item.firstVertex), item.firstInstance);
        return;
    }

    if (item.vertices > 0) {
        vkCmdDraw(commands, item.vertices, item.instances, item.firstVertex, item.firstInstance);
    }
}

/**
 **/
void Recorder::transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = from;
    barrier.newLayout = to;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (to == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        // two things have to have happened before the transition writes the image.
        //
        // COLOR_ATTACHMENT_OUTPUT is the stage the presenter waits the image-available
        // semaphore at, and a transition is a write: without that stage in the first scope
        // the barrier is not ordered after the wait, and the acquire's read of the image
        // races it. Synchronization validation reports that as WRITE_AFTER_READ against
        // vkAcquireNextImageKHR.
        //
        // FRAGMENT_SHADER is for a render target rather than the swapchain: there is one
        // image and two frames in flight, so the previous frame may still be sampling it. A
        // barrier's first scope reaches work already submitted to the queue, so naming the
        // stage that reads is what orders the two.
        barrier.srcStageMask =
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        // a write after a read needs the reads to have happened, not to be visible
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // a target changing hands: what the pass wrote has to be visible to the fragment
        // shader of whichever later pass samples it
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    } else {
        // presentation is not a pipeline stage - the semaphore it waits on is what
        // orders it, so the barrier only has to make the writes visible
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_NONE;
    }

    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commands, &dependency);
}

/**
 **/
void Recorder::transitionDepth(VkCommandBuffer commands, VkImage image) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    // nothing carries depth from one frame to the next, so what the last frame left is
    // not worth the barrier it would cost to preserve
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    // the previous frame's tests are what this waits on, and they run at both depth stages
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commands, &dependency);
}

};  // namespace v3d::render::realtime::vulkan
