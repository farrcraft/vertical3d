/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

#include "DepthBuffer.h"
#include "Device.h"
#include "Resources.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

/**
 * An image a pass draws into that is not the swapchain's, and that a later pass samples.
 *
 * The second half is the point: a target is created with sampled usage and is left in
 * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL by the recorder once the last pass drawing into
 * it has finished, so what one pass rendered is what the next one reads. A shadow map, a
 * scene rendered before it is graded, and a second view of one scene are all this.
 *
 * The extent is given rather than following the swapchain. A shadow map is sized by how
 * much detail it needs and not by the window; a target that should track the window is
 * recreated by the app when Engine3D::beginFrame reports a new size.
 *
 * The colour format is given too, and defaults to the swapchain's. A pipeline under dynamic
 * rendering is built against the format of what it draws into, so a pass drawing into a
 * target of a different format needs a pipeline built for that format - the pipeline is not
 * something a target can fix up at record time.
 *
 * A target owns its images and frees them, which is what separates it from everything in
 * Resources: those are built at load time and live until the context does, and a target is
 * thrown away and rebuilt whenever what it is sized against changes.
 **/
class RenderTarget final {
 public:
    /**
     * @param device the device to allocate on
     * @param width in pixels
     * @param height in pixels
     * @param colour the format of the colour image
     * @param depth whether to allocate a depth image the same size, for a pass that tests
     * @throw std::runtime_error if allocation fails, or if either dimension is zero
     **/
    RenderTarget(const boost::shared_ptr<Device>& device, uint32_t width, uint32_t height,
        VkFormat colour, bool depth = false);

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
     * against the old ones has to write it again. A Resources texture registered for the
     * old view refers to an image that no longer exists.
     *
     * @throw std::runtime_error if the new allocation fails
     **/
    void recreate(uint32_t width, uint32_t height);

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
     * The target described as something Resources can own, so that a draw item can name it
     * as a material's texture and sample what was rendered into it.
     *
     * The images belong to the target and not to Resources, so the registered copy has no
     * memory: registering it hands over a view and a sampler to bind, not an allocation to
     * free. That is why this returns a value rather than registering itself - what is
     * registered has to be understood as a reference to something with its own lifetime.
     **/
    Texture texture() const;

 private:
    /**
     **/
    void create(uint32_t width, uint32_t height);

    /**
     **/
    void destroy();

    boost::shared_ptr<Device> device_;
    VkFormat format_;
    VkImage image_;
    VkDeviceMemory memory_;
    VkImageView view_;
    VkSampler sampler_;
    VkExtent2D extent_;
    bool wantsDepth_;
    boost::shared_ptr<DepthBuffer> depth_;
};

};  // namespace v3d::render::realtime::vulkan
