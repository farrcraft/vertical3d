/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Recorder.h"

#include <api/render/realtime/vulkan/memory/Barriers.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "RenderTarget.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

namespace {

/**
 * Whether the pass at an index is the first of the frame to draw into the target it
 * names, or the last.
 *
 * A target is brought into the layout a pass attaches it in once, before the first pass
  * that writes it, and left readable after the last one, so two passes drawing into one
 * target cost one pair of barriers rather than two. Scanned rather than tallied because a
 * frame has a handful of passes, and a scan is easier to verify than a map that has to be
 * cleared every frame.
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
 * Whether any pass that writes the same target as passes[index] uses its depth. A target is
 * opened and closed once a frame, so its depth image is moved if any of its passes needs it.
 **/
bool depthWritten(const std::vector<boost::shared_ptr<Pass>>& passes, std::size_t index) {
    const boost::shared_ptr<RenderTarget>& target = passes[index]->target();
    return std::ranges::any_of(passes, [&target](const boost::shared_ptr<Pass>& other) {
        return other->target() == target && other->depth();
    });
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
    into.sampledDepth = offscreen->sampledDepth();
    into.format = offscreen->format();
    into.depthFormat = offscreen->depthFormat();
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
depthView(VK_NULL_HANDLE),
sampledDepth(false),
format(VK_FORMAT_UNDEFINED),
depthFormat(VK_FORMAT_UNDEFINED),
finalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
}

/**
 **/
Recorder::Bound::Bound() noexcept :
pipeline(nullptr),
frameSet(VK_NULL_HANDLE),
sceneSet(VK_NULL_HANDLE),
set(VK_NULL_HANDLE),
vertexBuffer(VK_NULL_HANDLE),
vertexBufferOffset(0),
indexBuffer(VK_NULL_HANDLE),
indexBufferOffset(0),
area{},
scissor{},
scissorSet(false),
into(nullptr) {
}

/**
 **/
void Recorder::record(VkCommandBuffer commands, const Frame& frame, const Target& target, const pipeline::Resources& resources,
    FrameUniforms* uniforms, Timings* timings) {
    // the acquired image comes back in whatever layout it was left in, and nothing in the
    // frame reads it, so undefined is the correct source layout and the cheapest one. A frame
    // given no image is one whose every pass names a target of its own
    if (target.image != VK_NULL_HANDLE) {
        memory::record(commands, {memory::colourForDrawing(target.image)});
    }

    if (frame.swapchainDepth() && target.depthImage != VK_NULL_HANDLE) {
        memory::record(commands, {memory::depthForDrawing(target.depthImage)});
    }

    // every pass drawing into a target before every pass reading it
    const std::vector<boost::shared_ptr<Pass>> passes = frame.ordered();
    for (std::size_t index = 0; index < passes.size(); ++index) {
        const boost::shared_ptr<Pass>& pass = passes[index];

        const Target into = resolve(*pass, target);
        const bool offscreen = static_cast<bool>(pass->target());

        if (offscreen && firstWrite(passes, index)) {
            openTarget(commands, depthWritten(passes, index), into);
        }

        if (timings != nullptr) {
            timings->open(commands, pass->name());
        }
        record(commands, *pass, into, resources, writeCamera(uniforms, *pass, into));
        if (timings != nullptr) {
            timings->close(commands);
        }

        if (offscreen && lastWrite(passes, index)) {
            closeTarget(commands, depthWritten(passes, index), into);
        }
    }

    if (target.image != VK_NULL_HANDLE) {
        memory::record(commands, {memory::colourAfterDrawing(target.image, target.finalLayout)});
    }
}

/**
 **/
void Recorder::openTarget(VkCommandBuffer commands, bool depth, const Target& into) {
    // undefined as the source layout: a target carries nothing from one frame to the next,
    // the same way the swapchain image and the depth buffer do not. The barrier still orders
    // this frame's writes after the reads the previous frame made of the same image
    if (into.image != VK_NULL_HANDLE) {
        memory::record(commands, {memory::colourForDrawing(into.image)});
    }
    if (depth && into.depthImage != VK_NULL_HANDLE) {
        memory::record(commands, {memory::depthForDrawing(into.depthImage)});
    }
}

/**
 **/
void Recorder::closeTarget(VkCommandBuffer commands, bool depth, const Target& into) {
    // every pass after the last one that wrote the target can sample it
    if (into.image != VK_NULL_HANDLE) {
        memory::record(commands, {memory::colourAfterDrawing(into.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)});
    }
    // and the same for its depth when it was created sampled: DEPTH_READ_ONLY_OPTIMAL lets a
    // later pass both sample it and depth test against it. A shadow map has only this half
    if (depth && into.sampledDepth && into.depthImage != VK_NULL_HANDLE) {
        memory::record(commands, {memory::depthForSampling(into.depthImage)});
    }
}

/**
 **/
void Recorder::record(VkCommandBuffer commands, const Pass& pass, const Target& target, const pipeline::Resources& resources,
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
    // a target with no colour image is one a depth-only pipeline draws into
    const bool hasColour = target.view != VK_NULL_HANDLE;
    rendering.colorAttachmentCount = hasColour ? 1 : 0;
    rendering.pColorAttachments = hasColour ? &colour : nullptr;
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
    bound.into = &target;
    // an item that names no clip of its own draws into all of this
    bound.area = area;
    bound.scissor = area;
    bound.scissorSet = true;
    for (const DrawItem* item : ordered) {
        record(commands, pass, *item, resources, frameSet, &bound);
    }

    vkCmdEndRendering(commands);
}

/**
 **/
void Recorder::check(const Pass& pass, const pipeline::Pipeline& pipeline, const Target& into) {
    if (pipeline.scene && pass.scene() == VK_NULL_HANDLE) {
        std::stringstream msg;
        msg << "The " << pass.name() << " pass draws with a pipeline that declares a scene at set 2, and names none";
        throw std::runtime_error(msg.str());
    }
    if (pipeline.biased && !pass.depthBias()) {
        std::stringstream msg;
        msg << "The " << pass.name() << " pass draws with a pipeline built with depth bias, and names no bias";
        throw std::runtime_error(msg.str());
    }
    // the pipeline's formats must match the target's. Validation reports a mismatch too, but
    // only with its layers on, and as a draw rather than a pass. The recorder attaches one
    // colour image at most, so only the first format is compared
    if (into.format != VK_FORMAT_UNDEFINED && !pipeline.colourFormats.empty() && pipeline.colourFormats.front() != into.format) {
        std::stringstream msg;
        msg << "The " << pass.name() << " pass draws into colour format " << into.format
            << " with a pipeline built for colour format " << pipeline.colourFormats.front();
        throw std::runtime_error(msg.str());
    }
    if (pass.depth() && into.depthFormat != VK_FORMAT_UNDEFINED && pipeline.depthFormat != VK_FORMAT_UNDEFINED &&
        pipeline.depthFormat != into.depthFormat) {
        std::stringstream msg;
        msg << "The " << pass.name() << " pass draws into depth format " << into.depthFormat
            << " with a pipeline built for depth format " << pipeline.depthFormat;
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
void Recorder::record(VkCommandBuffer commands, const Pass& pass, const DrawItem& item, const pipeline::Resources& resources,
    VkDescriptorSet frameSet, Bound* bound) {
    // the escape hatch, for work the item's fields cannot describe. It may record anything,
    // so nothing about what is bound is assumed to survive it
    if (item.record) {
        item.record(commands);
        const VkRect2D area = bound->area;
        const Target* into = bound->into;
        *bound = Bound();
        // it may have changed the scissor, so the next item sets one again
        bound->area = area;
        bound->into = into;
        return;
    }

    scissor(commands, item, bound);

    const pipeline::Pipeline* pipeline = resources.pipeline(item.pipeline);
    if (pipeline == nullptr || pipeline->pipeline == VK_NULL_HANDLE) {
        return;
    }

    if (pipeline != bound->pipeline) {
        check(pass, *pipeline, *bound->into);
        vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
        bound->pipeline = pipeline;
        // a different layout invalidates what was bound against the old one
        bound->frameSet = VK_NULL_HANDLE;
        bound->sceneSet = VK_NULL_HANDLE;
        bound->set = VK_NULL_HANDLE;
        if (pipeline->biased) {
            // dynamic state outlives a bind only into another pipeline that also declares
            // it dynamic, so a biased pipeline sets it every time it is bound
            const Pass::DepthBias& bias = *pass.depthBias();
            vkCmdSetDepthBias(commands, bias.constant, bias.clamp, bias.slope);
        }
    }

    if (frameSet != VK_NULL_HANDLE && frameSet != bound->frameSet) {
        // set 0 holds per frame data: the camera the whole pass draws through, so it is bound
        // here and never per item
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &frameSet, 0, nullptr);
        bound->frameSet = frameSet;
    }

    if (pipeline->scene && pass.scene() != bound->sceneSet) {
        // set 2 is the scene, shared by every lit item in the pass, so it is bound once for
        // the pass like set 0, and only for a pipeline that declares one
        VkDescriptorSet scene = pass.scene();
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 2, 1, &scene, 0, nullptr);
        bound->sceneSet = scene;
    }

    if (item.pushSize > 0 && pipeline->pushStages != 0) {
        vkCmdPushConstants(commands, pipeline->layout, pipeline->pushStages, 0, item.pushSize, item.push.data());
    }

    const pipeline::Material* material = resources.material(item.material);
    if (material != nullptr && material->set != VK_NULL_HANDLE && material->set != bound->set) {
        // set 1 holds per material data
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
void Recorder::scissor(VkCommandBuffer commands, const DrawItem& item, Bound* bound) {
    VkRect2D wanted = bound->area;
    if (item.scissored) {
        // clamped to the pass rather than trusted: an offset outside the image, or an
        // extent running past its edge, is a validation error and not a wrong picture
        const int32_t left = std::max(item.scissor.offset.x, bound->area.offset.x);
        const int32_t top = std::max(item.scissor.offset.y, bound->area.offset.y);
        const int64_t right = std::min(static_cast<int64_t>(item.scissor.offset.x) + item.scissor.extent.width,
            static_cast<int64_t>(bound->area.offset.x) + bound->area.extent.width);
        const int64_t bottom = std::min(static_cast<int64_t>(item.scissor.offset.y) + item.scissor.extent.height,
            static_cast<int64_t>(bound->area.offset.y) + bound->area.extent.height);

        wanted.offset.x = left;
        wanted.offset.y = top;
        wanted.extent.width = right > left ? static_cast<uint32_t>(right - left) : 0;
        wanted.extent.height = bottom > top ? static_cast<uint32_t>(bottom - top) : 0;
    }

    if (bound->scissorSet && wanted.offset.x == bound->scissor.offset.x && wanted.offset.y == bound->scissor.offset.y &&
        wanted.extent.width == bound->scissor.extent.width && wanted.extent.height == bound->scissor.extent.height) {
        return;
    }
    vkCmdSetScissor(commands, 0, 1, &wanted);
    bound->scissor = wanted;
    bound->scissorSet = true;
}

};  // namespace v3d::render::realtime::vulkan::frame
