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

#include "Barriers.h"
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

    upload(staging, *texture.image, VkExtent3D{width, height, 1});
    return texture;
}

/**
 **/
pipeline::Texture TextureFactory::volume(const unsigned char* texels, uint32_t width, uint32_t height, uint32_t depth) const {
    if (texels == nullptr || width == 0 || height == 0 || depth == 0) {
        throw std::runtime_error("A vulkan volume texture needs texels and a non-zero size");
    }
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width) * height * depth * 4;
    Buffer staging(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bytes);
    staging.write(texels, bytes);

    Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.depth = depth;
    spec.format = VK_FORMAT_R8G8B8A8_UNORM;
    spec.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    pipeline::Texture texture;
    texture.image = boost::make_shared<Image>(device_, spec);
    texture.sampler = sampler_;
    upload(staging, *texture.image, VkExtent3D{width, height, depth});
    return texture;
}

/**
 **/
void TextureFactory::upload(const Buffer& staging, const Image& image, const VkExtent3D& extent) const {
    VkBuffer source = staging.handle();
    VkImage target = image.handle();
    uploader_->oneShot([source, target, extent](VkCommandBuffer commands) {
        record(commands, {forUpload(target)});

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = extent;
        vkCmdCopyBufferToImage(commands, source, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        record(commands, {uploadedForSampling(target)});
    });
}

};  // namespace v3d::render::realtime::vulkan::memory
