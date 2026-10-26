/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "Allocation.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * An image, the memory it lives in, and the one view everything in the renderer reads or
 * draws it through.
 *
 * The three are made and destroyed together wherever an image is needed, in a texture, a
 * render target or a depth buffer, so they are one object. It is held by shared pointer
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
        /**
         * How many layers a 2D image has. An image of more than one is always arrayed. Ignored
         * by a 3D image, which has one
         **/
        uint32_t layers;
        /**
         * Whether view() is a 2D array view over every layer, as a sampler2DArray reads, rather
         * than a 2D view. Each layer also gets a 2D view of its own, from layerView()
         **/
        bool arrayed;
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
     * @return the view, for an attachment or a descriptor set: a 2D array view for an arrayed
     *         image, and a 2D or 3D view otherwise
     **/
    VkImageView view() const noexcept;

    /**
     * @return a 2D view of one layer, which an attachment that draws into that layer alone
     *         names. view() for an image that is not arrayed, and null for a layer it does not
     *         have
     **/
    VkImageView layerView(uint32_t layer) const noexcept;

    /**
     * @return how many layers the image has
     **/
    uint32_t layers() const noexcept;

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
    std::vector<VkImageView> layerViews_;  /**< one per layer, for an arrayed image only **/
};

};  // namespace v3d::render::realtime::vulkan::memory
