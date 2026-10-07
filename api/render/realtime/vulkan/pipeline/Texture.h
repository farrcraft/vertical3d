/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/memory/Image.h>

#include "Sampler.h"

#include <vulkan/vulkan.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * An image the shaders sample from, and the sampler they read it through.
 *
 * Both are shared rather than owned. A texture memory::TextureFactory built is the only
 * holder of its image; one registered from a frame::RenderTarget shares the target's, so
 * the image outlives whichever of the two lets go first. Either way a released texture is
 * destroyed by the last reference going, which the ring holds until no frame can read it.
 **/
struct Texture final {
    /**
     * What a descriptor write names to sample this texture, in the layout the image is left in
     * for sampling: DEPTH_READ_ONLY_OPTIMAL for a depth image, as the recorder leaves one, and
     * SHADER_READ_ONLY_OPTIMAL for a colour one.
     **/
    VkDescriptorImageInfo descriptor() const noexcept {
        VkDescriptorImageInfo info{};
        info.imageLayout = (image->spec().aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0
            ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = image->view();
        info.sampler = sampler->handle();
        return info;
    }

    boost::shared_ptr<memory::Image> image;
    boost::shared_ptr<Sampler> sampler;
};

};  // namespace v3d::render::realtime::vulkan::pipeline
