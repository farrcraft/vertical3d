/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Image.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "DepthBuffer.h"
#include "Ring.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * An image a pass draws into that is not the swapchain's, and that a later pass samples.
 *
 * A target is created with sampled usage, and the recorder leaves it in
 * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL once the last pass drawing into it has finished,
 * so a later pass can read what an earlier one rendered. A shadow map, a scene rendered
 * before it is graded, and a second view of one scene are all render targets.
 *
 * The extent is given rather than following the swapchain. A shadow map is sized by how
 * much detail it needs and not by the window; a target that should track the window is
 * recreated by the app when Engine3D::beginFrame reports a new size.
 *
 * The colour format is given too, and is not taken from the swapchain. A pipeline under
 * dynamic rendering is built against the format of what it draws into, so a pass drawing
 * into a target of a different format needs a pipeline built for that format. A target
 * cannot adapt a pipeline at record time.
 *
 * A target's images are thrown away and rebuilt whenever what it is sized against changes,
 * and a frame still in flight may be drawing into them or reading them when that happens.
 * What it releases, on a resize or when it is destroyed, therefore goes to the ring rather
 * than being destroyed at once.
 *
 * A target holds one image, or one per frame in flight. With one per frame, a pass draws
 * into current() and what the previous frame drew is still readable at previous(). One
 * image cannot do that, because it would be read as last frame and written as this one at
 * once. The accessors that take no slot return current()'s.
 **/
class RenderTarget final {
 public:
    /**
     * @param device the device to allocate on
     * @param ring the frames in flight, which hold back the images a resize lets go of
     * @param width in pixels
     * @param height in pixels
     * @param colour the format of the colour image, or VK_FORMAT_UNDEFINED for none - a
     *        target with sampled depth and no colour is what a shadow map draws into, and
     *        every colour accessor then answers null
     * @param depth whether to allocate a depth image the same size, for a pass that tests
     * @param sampledDepth whether that depth image is also read by a later pass, as a
     *        shadow map is. It costs a sampler and can change which depth format
     *        the device gives, so a pipeline drawing into this has to be built against
     *        depthFormat() rather than against DepthBuffer::chooseFormat's default
     * @param images one, or the ring's frames in flight for a target a pass reads the previous
     *        frame of. Each image of a target with more than one starts cleared and readable,
     *        so previous() can be read on the first frame
     * @throw std::runtime_error if allocation fails, if either dimension is zero, if there
     *        is no colour and no sampled depth, which would be a target with nothing to read,
     *        or if images is neither one nor the frames in flight
     **/
    RenderTarget(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring,
        uint32_t width, uint32_t height, VkFormat colour, bool depth = false, bool sampledDepth = false,
        uint32_t images = 1);

    /**
     **/
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    /**
     * Throw the images away and build them at the new size, keeping the formats so that no
     * pipeline built against this target has to be rebuilt.
     *
     * The view and the sampler are new, so anything holding a descriptor set written
     * against the old ones has to write it again. A pipeline::Resources texture registered
     * from the old images keeps them alive and goes on naming them, so it has to be
     * released and the target registered again.
     *
     * The new images are built before the old ones are released. When this throws, the target
     * keeps its old images, size and views, and stays usable.
     *
     * @throw std::runtime_error if either dimension is zero or the new allocation fails
     **/
    void recreate(uint32_t width, uint32_t height);

    /**
     * @return how many images the target holds
     **/
    uint32_t images() const noexcept;

    /**
     * @return the slot the frame being built draws into, which is chosen by the ring's frame
     **/
    uint32_t current() const noexcept;

    /**
     * @return the slot the frame before this one drew into, which is current() for a target of
     *         one image
     **/
    uint32_t previous() const noexcept;

    /**
     * @return the colour image, for the layout transitions either side of a pass
     **/
    VkImage image() const noexcept;

    /**
     * @return the view a pass attaches as its colour attachment, and that a descriptor
     *         set samples
     **/
    VkImageView view() const noexcept;

    /**
     * @return the sampler a descriptor set reads the view through
     **/
    VkSampler sampler() const noexcept;

    /**
     * @return the colour format, which a pipeline drawing into this has to be built against
     **/
    VkFormat format() const noexcept;

    /**
     * @return the size of the images
     **/
    const VkExtent2D& extent() const noexcept;

    /**
     * @return the depth image, or null when the target was made without one
     **/
    VkImage depthImage() const noexcept;

    /**
     * @return the depth attachment's view, or null when there is none
     **/
    VkImageView depthView() const noexcept;

    /**
     * @return the depth format, or VK_FORMAT_UNDEFINED when there is no depth image
     **/
    VkFormat depthFormat() const noexcept;

    /**
     * The recorder leaves a sampled depth image in a readable layout after its last pass.
     *
     * @return whether the depth image can be read as well as written
     **/
    bool sampledDepth() const noexcept;

    /**
     * One slot's depth image described as something pipeline::Resources can own, so that a draw
     * item can name it as a material's texture and sample what was rendered into it. This is
     * how a shadow map is read.
     *
     * The same sharing rules as texture() apply. Its images are empty when the target carries
     * no depth, was not built to have it sampled, or has no such slot. A descriptor set
     * written against those would read an image with no sampled usage, which only the
     * validation layer reports.
     **/
    pipeline::Texture depthTexture(uint32_t slot = 0) const;

    /**
     * One slot described as something pipeline::Resources can own, so that a draw item can name
     * it as a material's texture and sample what was rendered into it. A target of more than
     * one image is registered once per slot, and a reader names current() or previous() each
     * frame.
     *
     * What is registered shares the target's image and sampler rather than copying them, so
     * whichever of the target and the registration releases them last frees them. This
     * returns a value rather than registering itself, because the caller owns the
     * registration and releases it.
     **/
    pipeline::Texture texture(uint32_t slot = 0) const;

 private:
    /**
     * One frame's images: the colour, and the depth when the target has one.
     **/
    struct Slot final {
        boost::shared_ptr<memory::Image> image;
        boost::shared_ptr<DepthBuffer> depth;
    };

    /**
     * Build a full set of images at the given size and replace the current set with it. The
     * current set is only released once every new image exists.
     **/
    void create(uint32_t width, uint32_t height);

    /**
     * A colour image of the target's format.
     **/
    boost::shared_ptr<memory::Image> createColour(uint32_t width, uint32_t height) const;

    /**
     * Clear every slot and leave it in the layout a reader samples it in, as though a pass had
     * drawn into it, so that a frame reading previous() before anything has reads a defined
     * image rather than one in an undefined layout.
     **/
    void ready(const std::vector<Slot>& slots, const VkExtent2D& extent) const;

    /**
     **/
    void destroy();

    /**
     * @return the current slot, or a slot with no images when the target holds none. A
     *         constructed target always holds at least one.
     **/
    const Slot& slot() const noexcept;

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Ring> ring_;
    VkFormat format_;
    uint32_t images_;
    std::vector<Slot> slots_;
    boost::shared_ptr<pipeline::Sampler> sampler_;
    VkExtent2D extent_;
    bool wantsDepth_;
    bool sampledDepth_;
};

};  // namespace v3d::render::realtime::vulkan::frame
