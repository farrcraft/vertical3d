/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>

#include "Allocator.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * An image, the memory it lives in, and the one view everything in the renderer reads or
 * draws it through.
 *
 * The three are made and destroyed together everywhere an image is wanted - a texture, a
 * render target, a depth buffer - so they are one object. It is held by shared pointer
 * because more than one thing can need it alive: a target and the texture registered from
 * it, or a frame still in flight after its owner let go.
 *
 * Always optimally tiled, one sample and one array layer, created in
 * VK_IMAGE_LAYOUT_UNDEFINED. Whoever writes it first moves it out of that layout.
 **/
class Image final {
 public:
    /**
     * What to make.
     **/
    struct Spec final {
        Spec() noexcept;

        uint32_t width;
        uint32_t height;
        uint32_t depth;                 /**< above 1 makes a 3D image and a 3D view **/
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageAspectFlags aspect;      /**< the aspects the view names **/
        uint32_t mipLevels;
        VkComponentMapping components;  /**< the view's swizzle, identity unless set **/
    };

    /**
     * @param device the device to create the image on
     * @param spec what to create
     * @throw std::runtime_error if a dimension is zero, or creation or allocation fails
     **/
    Image(const boost::shared_ptr<device::Device>& device, const Spec& spec);

    /**
     **/
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    /**
     * @return the image, for barriers and copies
     **/
    VkImage handle() const noexcept;

    /**
     * @return the view, for an attachment or a descriptor set
     **/
    VkImageView view() const noexcept;

    /**
     * @return what the image was made from
     **/
    const Spec& spec() const noexcept;

    /**
     * @return the width and height
     **/
    VkExtent2D extent() const noexcept;

 private:
    /**
     **/
    void destroy() noexcept;

    boost::shared_ptr<device::Device> device_;
    Spec spec_;
    VkImage image_;
    Allocation memory_;
    VkImageView view_;
};

};  // namespace v3d::render::realtime::vulkan::memory
