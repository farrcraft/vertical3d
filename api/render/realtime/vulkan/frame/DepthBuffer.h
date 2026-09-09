/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The depth image a scene pass tests and writes against.
 *
 * There is one of these per context rather than one per pass: every pass draws into the
 * swapchain image, so every pass wanting depth wants it at the same size, and a pass that
 * shares the buffer with the pass before it can keep what that one left. It is sized by
 * the swapchain and rebuilt with it.
 *
 * A window with no area has no depth buffer, the same way it has no swapchain - valid()
 * says so, and nothing is drawn while that is true.
 *
 * A buffer can also be made sampled, which is what a shadow map is: the image then carries
 * a sampler and a usage a descriptor set can bind, and the recorder leaves it in a read
 * only layout once the pass writing it has finished. That is asked for at construction and
 * not switched on later, because it changes which format the device will give - see
 * chooseFormat.
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
     * Clamped to a white border rather than to an edge: what a shadow map is asked about
     * outside the light's frustum should come back lit, and a clamped edge answers with
     * whatever the nearest texel happened to hold.
     **/
    VkSampler sampler() const noexcept;

    /**
     * @return the first depth format the device can use as an optimally tiled attachment
     *
     * Public because a pipeline is built against the format long before anything asks
     * for the image - the buffer itself is only allocated once a pass wants one.
     *
     * A format that will also be sampled is a different question and can be a different
     * answer, so a pipeline drawing into a sampled buffer has to be built against the
     * format this gives for one.
     *
     * @param sampled whether the format also has to be readable through a descriptor set
     * @throw std::runtime_error if the device offers none, which no conformant one does
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
    VkImage image_;
    VkDeviceMemory memory_;
    VkImageView view_;
    VkSampler sampler_;
    VkExtent2D extent_;
    bool sampled_;
};

};  // namespace v3d::render::realtime::vulkan::frame
