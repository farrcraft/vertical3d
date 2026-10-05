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

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The depth image a scene pass tests and writes against.
 *
 * A context keeps one of these for the passes that draw into the swapchain image, rather than
 * one per pass. Those passes need depth at the same size, and a pass that shares the buffer
 * with the pass before it can keep what that one left. It is sized by the swapchain and
 * rebuilt with it. A render target with depth holds one of its own, at the target's size.
 *
 * A window with no area has no depth buffer, the same way it has no swapchain - valid()
 * says so, and nothing is drawn while that is true.
 *
 * A buffer can also be made sampled, as a shadow map is. Its format must then support
 * SAMPLED_IMAGE as well as DEPTH_STENCIL_ATTACHMENT in optimal tiling; a device can offer the
 * second without the first. The image gets the sampled usage bit and a sampler, and the
 * recorder moves it to DEPTH_READ_ONLY_OPTIMAL for the passes after the one that writes it.
 * Sampling is chosen at construction and cannot be switched on later, because it can change
 * which format the device gives - see chooseFormat.
 **/
class DepthBuffer final {
 public:
    /**
     * @param device the device to allocate the image on
     * @param width in pixels, matching the swapchain's extent
     * @param height in pixels, matching the swapchain's extent
     * @param sampled whether a later pass will read this, which costs a sampler, a usage
     *        bit and a narrower choice of format
     * @throw std::runtime_error if the device offers no depth format, or allocation fails
     **/
    DepthBuffer(const boost::shared_ptr<device::Device>& device, uint32_t width, uint32_t height,
        bool sampled = false);

    /**
     **/
    ~DepthBuffer();

    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer& operator=(const DepthBuffer&) = delete;

    /**
     * Throw the image away and build one at the new size. The format never changes, so
     * no pipeline built against it has to be rebuilt.
     **/
    void recreate(uint32_t width, uint32_t height);

    /**
     * @return whether there is an image to draw into
     **/
    bool valid() const noexcept;

    /**
     * @return the underlying image, for the layout transition ahead of a frame
     **/
    VkImage image() const noexcept;

    /**
     * @return the view attached as the depth attachment
     **/
    VkImageView view() const noexcept;

    /**
     * @return the format the image was created with, which a pipeline is built against
     **/
    VkFormat format() const noexcept;

    /**
     * @return the size of the image
     **/
    const VkExtent2D& extent() const noexcept;

    /**
     * @return whether the format carries a stencil aspect, which a barrier has to name
     **/
    bool stencil() const noexcept;

    /**
     * @return whether this was built to be read as well as written
     **/
    bool sampled() const noexcept;

    /**
     * @return the sampler a descriptor set reads the view through, or null when this was
     *         not built sampled
     *
     * Clamped to a white border rather than to an edge: a shadow map lookup outside the
     * light's frustum should return lit, and an edge clamp would return whatever the nearest
     * texel held.
     **/
    VkSampler sampler() const noexcept;

    /**
     * The image described as something pipeline::Resources can hold, sharing it rather than
     * copying it, so that a draw item can sample what a pass tested against.
     *
     * @return the image and its sampler, or an empty texture when this was not built sampled
     *         or has no image
     **/
    pipeline::Texture texture() const;

    /**
     * @return the first depth format the device can use as an optimally tiled attachment
     *
     * Public because a pipeline is built against the format long before anything requests
     * the image - the buffer itself is only allocated once a pass needs one.
     *
     * A format that will also be sampled must support SAMPLED_IMAGE too, and can differ, so
     * a pipeline drawing into a sampled buffer has to be built against the format this
     * returns for one.
     *
     * @param sampled whether the format also has to be readable through a descriptor set
     * @throw std::runtime_error if the device offers none, which a conformant device never does
     **/
    static VkFormat chooseFormat(VkPhysicalDevice device, bool sampled = false);

 private:
    /**
     **/
    void create(uint32_t width, uint32_t height);

    /**
     **/
    void destroy();

    boost::shared_ptr<device::Device> device_;
    VkFormat format_;
    boost::shared_ptr<memory::Image> image_;
    boost::shared_ptr<pipeline::Sampler> sampler_;
    VkExtent2D extent_;
    bool sampled_;
};

};  // namespace v3d::render::realtime::vulkan::frame
