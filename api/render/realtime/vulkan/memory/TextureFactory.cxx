/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFactory.h"

#include <api/image/Image.h>

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Buffer.h"
#include "Image.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
TextureFactory::TextureFactory(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader) :
    device_(device),
    uploader_(uploader) {
    // the default: linear, because a glyph atlas is sampled at whatever size the text is drawn
    // at and a sprite at whatever size the window is, and clamped, because a region's
    // neighbour in an atlas is a different glyph and wrapping would bleed it in
    sampler_ = boost::make_shared<pipeline::Sampler>(device_, pipeline::Sampler::Spec());
}

/**
 **/
TextureFactory::~TextureFactory() {
}

/**
 **/
pipeline::Texture TextureFactory::create(const boost::shared_ptr<v3d::image::Image>& image, Encoding encoding) const {
    if (!image) {
        throw std::runtime_error("A vulkan texture needs an image to be created from");
    }
    // bpp really is bits per pixel here, whatever the name suggests: the loaders set it
    // to 24 or 32, and a glyph atlas of depth 1 to 8
    return create(image->data(), image->width(), image->height(), image->bpp() / 8, encoding);
}

/**
 **/
pipeline::Texture TextureFactory::create(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels,
    Encoding encoding) const {
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
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    if (coverage) {
        format = VK_FORMAT_R8_UNORM;
    } else if (encoding == Encoding::Srgb) {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    }
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

    Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.format = format;
    spec.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (coverage) {
        // a glyph atlas carries coverage in its one channel, and the quad shader
        // multiplies the vertex colour by whatever it samples - so hand it white
        spec.components.r = VK_COMPONENT_SWIZZLE_ONE;
        spec.components.g = VK_COMPONENT_SWIZZLE_ONE;
        spec.components.b = VK_COMPONENT_SWIZZLE_ONE;
        spec.components.a = VK_COMPONENT_SWIZZLE_R;
    }

    pipeline::Texture texture;
    texture.image = boost::make_shared<Image>(device_, spec);
    texture.sampler = sampler_;

    VkBuffer source = staging.handle();
    VkImage image = texture.image->handle();
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
