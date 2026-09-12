/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFactory.h"

#include <api/image/Image.h>
#include <api/render/realtime/vulkan/device/Result.h>

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Buffer.h"
#include "Memory.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
TextureFactory::TextureFactory(const boost::shared_ptr<device::Device>& device) :
    device_(device) {
    uploader_ = boost::make_shared<Uploader>(device_);
}

/**
 **/
TextureFactory::~TextureFactory() {
}

/**
 **/
pipeline::Texture TextureFactory::create(const boost::shared_ptr<v3d::image::Image>& image) const {
    if (!image) {
        throw std::runtime_error("A vulkan texture needs an image to be created from");
    }
    // bpp really is bits per pixel here, whatever the name suggests: the loaders set it
    // to 24 or 32, and a glyph atlas of depth 1 to 8
    return create(image->data(), image->width(), image->height(), image->bpp() / 8);
}

/**
 **/
pipeline::Texture TextureFactory::create(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels) const {
    if (pixels == nullptr || width == 0 || height == 0) {
        throw std::runtime_error("A vulkan texture needs pixels and a non-zero size");
    }
    if (channels != 1 && channels != 3 && channels != 4) {
        std::stringstream msg;
        msg << "A vulkan texture cannot be created from " << channels << " channel pixels";
        throw std::runtime_error(msg.str());
    }

    const bool coverage = channels == 1;
    const uint32_t uploaded = coverage ? 1 : 4;
    const VkFormat format = coverage ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width) * height * uploaded;

    Buffer staging(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bytes);
    if (channels == 3) {
        // no device has to support a three channel format, so widen on the way in
        std::vector<unsigned char> widened(static_cast<std::size_t>(bytes));
        const std::size_t count = static_cast<std::size_t>(width) * height;
        for (std::size_t pixel = 0; pixel < count; pixel++) {
            widened[pixel * 4 + 0] = pixels[pixel * 3 + 0];
            widened[pixel * 4 + 1] = pixels[pixel * 3 + 1];
            widened[pixel * 4 + 2] = pixels[pixel * 3 + 2];
            widened[pixel * 4 + 3] = 0xFF;
        }
        staging.write(widened.data(), bytes);
    } else {
        staging.write(pixels, bytes);
    }

    pipeline::Texture texture;
    texture.extent.width = width;
    texture.extent.height = height;

    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device_->handle(), &info, nullptr, &texture.image);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create a vulkan image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = device_->allocator().bind(texture.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture.memory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(device_->handle(), texture.image, nullptr);
        std::stringstream msg;
        msg << "Unable to allocate memory for a vulkan image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkBuffer source = staging.handle();
    VkImage image = texture.image;
    uploader_->oneShot([source, image, width, height](VkCommandBuffer commands) {
        transition(commands, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent.width = width;
        copy.imageExtent.height = height;
        copy.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(commands, source, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        transition(commands, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = texture.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    if (coverage) {
        // a glyph atlas carries coverage in its one channel, and the quad shader
        // multiplies the vertex colour by whatever it samples - so hand it white
        view.components.r = VK_COMPONENT_SWIZZLE_ONE;
        view.components.g = VK_COMPONENT_SWIZZLE_ONE;
        view.components.b = VK_COMPONENT_SWIZZLE_ONE;
        view.components.a = VK_COMPONENT_SWIZZLE_R;
    }
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device_->handle(), &view, nullptr, &texture.view);
    if (result != VK_SUCCESS) {
        device_->allocator().free(&texture.memory);
        vkDestroyImage(device_->handle(), texture.image, nullptr);
        std::stringstream msg;
        msg << "Unable to create a vulkan image view - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // linear everywhere: a glyph atlas is sampled at whatever size the text is drawn at
    // and a sprite at whatever size the window is, so nearest would alias on both
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // clamping rather than repeating, because a region's neighbour in an atlas is a
    // different glyph and wrapping would bleed it in
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler.maxLod = VK_LOD_CLAMP_NONE;

    result = vkCreateSampler(device_->handle(), &sampler, nullptr, &texture.sampler);
    if (result != VK_SUCCESS) {
        vkDestroyImageView(device_->handle(), texture.view, nullptr);
        device_->allocator().free(&texture.memory);
        vkDestroyImage(device_->handle(), texture.image, nullptr);
        std::stringstream msg;
        msg << "Unable to create a vulkan sampler - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    return texture;
}

/**
 **/
void TextureFactory::transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = from;
    barrier.newLayout = to;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    if (to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    }

    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commands, &dependency);
}

};  // namespace v3d::render::realtime::vulkan::memory
