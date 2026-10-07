/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderTarget.h"

#include <api/render/realtime/vulkan/memory/Barriers.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>

#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

namespace {

/**
 * Clear what is attached to transparent black and the far plane, with no draws.
 **/
void clear(VkCommandBuffer commands, VkImageView colour, VkImageView depth, const VkExtent2D& extent) {
    VkRenderingAttachmentInfo colourAttachment{};
    colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colourAttachment.imageView = colour;
    colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depth;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // the far plane, so a shadow map read before anything was drawn reports no shadow
    depthAttachment.clearValue.depthStencil.depth = 1.0f;

    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea.extent = extent;
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = colour != VK_NULL_HANDLE ? 1 : 0;
    rendering.pColorAttachments = colour != VK_NULL_HANDLE ? &colourAttachment : nullptr;
    rendering.pDepthAttachment = depth != VK_NULL_HANDLE ? &depthAttachment : nullptr;
    vkCmdBeginRendering(commands, &rendering);
    vkCmdEndRendering(commands);
}

/**
 * Clear a slot's images. Each goes from undefined into the layout a pass draws in, and then
 * into the layout the recorder leaves a target in after its last pass. A slot no pass has
 * drawn into then looks like one a pass has. A null image is one the slot does not have.
 **/
void readied(VkCommandBuffer commands, VkImage colour, VkImageView colourView, VkImage depth, VkImageView depthView,
    const VkExtent2D& extent) {
    if (colour != VK_NULL_HANDLE) {
        memory::record(commands, {memory::colourForDrawing(colour)});
    }
    if (depth != VK_NULL_HANDLE) {
        memory::record(commands, {memory::depthForDrawing(depth)});
    }
    clear(commands, colourView, depthView, extent);
    if (colour != VK_NULL_HANDLE) {
        memory::record(commands, {memory::colourAfterDrawing(colour, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)});
    }
    if (depth != VK_NULL_HANDLE) {
        memory::record(commands, {memory::depthForSampling(depth)});
    }
}

};  // namespace

/**
 **/
RenderTarget::RenderTarget(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring,
    uint32_t width, uint32_t height, VkFormat colour, bool depth, bool sampledDepth, uint32_t images) :
    device_(device),
    ring_(ring),
    format_(colour),
    images_(images),
    extent_(),
    wantsDepth_(depth),
    sampledDepth_(sampledDepth) {
    // a slot is chosen by the ring's frame, so any other count would make previous() a frame
    // that is not the one before
    if (images_ != 1 && images_ != ring_->framesInFlight()) {
        throw std::runtime_error("A vulkan render target holds one image or one per frame in flight");
    }
    create(width, height);
}

/**
 **/
RenderTarget::~RenderTarget() {
    destroy();
}

/**
 **/
void RenderTarget::recreate(uint32_t width, uint32_t height) {
    create(width, height);
}

/**
 **/
void RenderTarget::create(uint32_t width, uint32_t height) {
    // unlike a swapchain image, a target is created at a size the caller chose, so a
    // dimension of zero is a mistake rather than a minimized window
    if (width == 0 || height == 0) {
        throw std::runtime_error("A vulkan render target cannot have a zero dimension");
    }

    if (format_ == VK_FORMAT_UNDEFINED && !(wantsDepth_ && sampledDepth_)) {
        throw std::runtime_error("A vulkan render target with no colour image needs a sampled depth image");
    }

    // the new images are built in full before the old ones are let go, so a create that throws
    // leaves the target holding what it held before
    std::vector<Slot> slots(images_);
    for (Slot& slot : slots) {
        if (format_ != VK_FORMAT_UNDEFINED) {
            slot.image = createColour(width, height);
        }
        if (wantsDepth_) {
            // a target's depth is a DepthBuffer, the same class the swapchain passes use
            slot.depth = boost::make_shared<DepthBuffer>(device_, width, height, sampledDepth_);
        }
    }

    boost::shared_ptr<pipeline::Sampler> sampler;
    if (format_ != VK_FORMAT_UNDEFINED) {
        // the default: linear, because a target is read at whatever size the pass reading it
        // draws, and clamped, because sampling past its edge is reaching outside what was
        // rendered. One serves every slot, since they differ only in what was drawn
        sampler = boost::make_shared<pipeline::Sampler>(device_, pipeline::Sampler::Spec());
    }

    VkExtent2D extent{};
    extent.width = width;
    extent.height = height;

    if (images_ > 1) {
        ready(slots, extent);
    }

    destroy();
    slots_ = std::move(slots);
    sampler_ = std::move(sampler);
    extent_ = extent;
}

/**
 **/
boost::shared_ptr<memory::Image> RenderTarget::createColour(uint32_t width, uint32_t height) const {
    memory::Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.format = format_;
    // a pass draws into a target and a later pass reads it. TRANSFER_SRC lets frame::Capture
    // copy one out. It is always granted, as SAMPLED is, and its only cost is any compression
    // a desktop driver disables for it
    spec.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    return boost::make_shared<memory::Image>(device_, spec);
}

/**
 **/
void RenderTarget::ready(const std::vector<Slot>& slots, const VkExtent2D& extent) const {
    // a clear by rendering rather than by transfer, because attachment usage is what every
    // target's images already have
    memory::Uploader uploader(device_);
    uploader.oneShot([&slots, &extent](VkCommandBuffer commands) {
        for (const Slot& slot : slots) {
            const bool depth = slot.depth && slot.depth->sampled();
            readied(commands, slot.image ? slot.image->handle() : VK_NULL_HANDLE, slot.image ? slot.image->view() : VK_NULL_HANDLE,
                depth ? slot.depth->image() : VK_NULL_HANDLE, depth ? slot.depth->view() : VK_NULL_HANDLE, extent);
        }
    });
}

/**
 **/
void RenderTarget::destroy() {
    if (!slots_.empty()) {
        // a frame in flight may still be drawing into these or sampling them
        ring_->retire([slots = std::move(slots_), sampler = std::move(sampler_)]() mutable {
            slots.clear();
            sampler.reset();
        });
    }
    // moved from, which leaves them valid but unspecified
    slots_.clear();
    sampler_.reset();
    extent_.width = 0;
    extent_.height = 0;
}

/**
 **/
uint32_t RenderTarget::images() const noexcept {
    return images_;
}

/**
 **/
uint32_t RenderTarget::current() const noexcept {
    return ring_->frame() % images_;
}

/**
 **/
uint32_t RenderTarget::previous() const noexcept {
    return (ring_->frame() + images_ - 1) % images_;
}

/**
 **/
const RenderTarget::Slot& RenderTarget::slot() const noexcept {
    // every accessor then answers null rather than reading past the end
    static const Slot none;
    if (slots_.empty()) {
        return none;
    }
    return slots_[current()];
}

/**
 **/
VkImage RenderTarget::image() const noexcept {
    return slot().image ? slot().image->handle() : VK_NULL_HANDLE;
}

/**
 **/
VkImageView RenderTarget::view() const noexcept {
    return slot().image ? slot().image->view() : VK_NULL_HANDLE;
}

/**
 **/
VkSampler RenderTarget::sampler() const noexcept {
    return sampler_ ? sampler_->handle() : VK_NULL_HANDLE;
}

/**
 **/
VkFormat RenderTarget::format() const noexcept {
    return format_;
}

/**
 **/
const VkExtent2D& RenderTarget::extent() const noexcept {
    return extent_;
}

/**
 **/
VkImage RenderTarget::depthImage() const noexcept {
    return slot().depth ? slot().depth->image() : VK_NULL_HANDLE;
}

/**
 **/
VkImageView RenderTarget::depthView() const noexcept {
    return slot().depth ? slot().depth->view() : VK_NULL_HANDLE;
}

/**
 **/
VkFormat RenderTarget::depthFormat() const noexcept {
    return slot().depth ? slot().depth->format() : VK_FORMAT_UNDEFINED;
}

/**
 **/
bool RenderTarget::sampledDepth() const noexcept {
    return sampledDepth_ && slot().depth && slot().depth->sampled();
}

/**
 **/
pipeline::Texture RenderTarget::depthTexture(uint32_t slot) const {
    if (!sampledDepth() || slot >= slots_.size()) {
        return pipeline::Texture();
    }
    return slots_[slot].depth->texture();
}

/**
 **/
pipeline::Texture RenderTarget::texture(uint32_t slot) const {
    pipeline::Texture texture;
    if (slot < slots_.size()) {
        texture.image = slots_[slot].image;
        texture.sampler = sampler_;
    }
    return texture;
}

};  // namespace v3d::render::realtime::vulkan::frame
