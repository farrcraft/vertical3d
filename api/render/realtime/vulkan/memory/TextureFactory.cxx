/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFactory.h"

#include <api/image/Image.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Barriers.h"
#include "Buffer.h"
#include "Image.h"
#include "Uploader.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::memory {

namespace {

/**
 * The bytes of a texel of four 32 bit floats.
 **/
constexpr VkDeviceSize FLOAT_TEXEL_BYTES = 4 * sizeof(float);

};  // namespace

/**
 **/
TextureFactory::TextureFactory(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader) :
    device_(device),
    uploader_(uploader) {
    // the default. It is linear, because a glyph atlas is sampled at whatever size the text is
    // drawn at and a sprite at whatever size the window is. It is clamped, so a read past one
    // edge of the image does not wrap around to the opposite edge
    sampler(pipeline::Sampler::Spec());
}

/**
 **/
TextureFactory::~TextureFactory() {
}

/**
 **/
pipeline::Texture TextureFactory::create(const boost::shared_ptr<v3d::image::Image>& image, Encoding encoding,
    const pipeline::Sampler::Spec& sampler) const {
    if (!image) {
        throw std::runtime_error("A vulkan texture needs an image to be created from");
    }
    // bpp is bits per pixel. The loaders set it to 24 or 32, and a glyph atlas of depth 1
    // sets it to 8
    return create({.data = image->data(), .width = image->width(), .height = image->height(), .channels = static_cast<uint32_t>(image->bpp() / 8)}, encoding, sampler);
}

/**
 **/
pipeline::Texture TextureFactory::create(const Pixels& source, Encoding encoding, const pipeline::Sampler::Spec& sampler) const {
    const uint32_t width = source.width;
    const uint32_t height = source.height;
    const uint32_t channels = source.channels;
    if (source.data == nullptr || width == 0 || height == 0) {
        throw std::runtime_error("A vulkan texture needs pixels and a non-zero size");
    }
    if (channels != 1 && channels != 3 && channels != 4) {
        std::stringstream msg;
        msg << "A vulkan texture cannot be created from " << channels << " channel pixels";
        throw std::runtime_error(msg.str());
    }

    const bool coverage = channels == 1;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    if (coverage) {
        format = VK_FORMAT_R8_UNORM;
    } else if (encoding == Encoding::Srgb) {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    const boost::shared_ptr<Buffer> staging = stage(source);

    Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.format = format;
    spec.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (coverage) {
        // the one channel is read as alpha, and rgb as white. The quad shader multiplies a
        // coverage mask's sample into the vertex colour, and reads a glyph atlas's distance
        // from alpha
        spec.components.r = VK_COMPONENT_SWIZZLE_ONE;
        spec.components.g = VK_COMPONENT_SWIZZLE_ONE;
        spec.components.b = VK_COMPONENT_SWIZZLE_ONE;
        spec.components.a = VK_COMPONENT_SWIZZLE_R;
    }

    pipeline::Texture texture;
    texture.image = boost::make_shared<Image>(device_, spec);
    texture.sampler = this->sampler(sampler);

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
    const boost::shared_ptr<Buffer> staging = boost::make_shared<Buffer>(device_,
        BufferSpec{.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .bytes = bytes});
    staging->write(texels, bytes);

    Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.depth = depth;
    spec.format = VK_FORMAT_R8G8B8A8_UNORM;
    spec.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    pipeline::Texture texture;
    texture.image = boost::make_shared<Image>(device_, spec);
    texture.sampler = sampler(pipeline::Sampler::Spec());
    upload(staging, *texture.image, VkExtent3D{width, height, depth});
    return texture;
}

/**
 **/
boost::shared_ptr<pipeline::Sampler> TextureFactory::sampler(const pipeline::Sampler::Spec& spec) const {
    // a scene names a handful of specs, so a search is cheaper than keeping them ordered
    for (const auto& [made, existing] : samplers_) {
        if (made == spec) {
            return existing;
        }
    }
    samplers_.emplace_back(spec, boost::make_shared<pipeline::Sampler>(device_, spec));
    return samplers_.back().second;
}

/**
 **/
bool TextureFactory::fits(const Image& image, const Pixels& source, uint32_t x, uint32_t y) {
    const Image::Spec& spec = image.spec();
    const bool texture = spec.depth <= 1 && spec.aspect == VK_IMAGE_ASPECT_COLOR_BIT && (spec.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0;
    const bool coverage = spec.format == VK_FORMAT_R8_UNORM;
    const bool colour = spec.format == VK_FORMAT_R8G8B8A8_UNORM || spec.format == VK_FORMAT_R8G8B8A8_SRGB;
    const bool channels = coverage ? source.channels == 1 : (colour && (source.channels == 3 || source.channels == 4));
    return texture && channels && inside(image, x, y, source.width, source.height) && source.data != nullptr;
}

/**
 **/
bool TextureFactory::inside(const Image& image, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    const Image::Spec& spec = image.spec();
    // compared as differences, so a place far outside the image cannot wrap around into it
    return width > 0 && height > 0 && x < spec.width && y < spec.height && width <= spec.width - x && height <= spec.height - y;
}

/**
 **/
pipeline::Texture TextureFactory::create(const Texels& source) const {
    if (source.data == nullptr || source.width == 0 || source.height == 0) {
        throw std::runtime_error("A vulkan float texture needs texels and a non-zero size");
    }
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(source.width) * source.height * FLOAT_TEXEL_BYTES;
    const boost::shared_ptr<Buffer> staging = boost::make_shared<Buffer>(device_,
        BufferSpec{.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .bytes = bytes});
    staging->write(source.data, bytes);

    Image::Spec spec;
    spec.width = source.width;
    spec.height = source.height;
    spec.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    spec.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    // a float format need not filter, and the texels are fetched rather than sampled anyway
    pipeline::Sampler::Spec nearest;
    nearest.filter = VK_FILTER_NEAREST;
    nearest.mipmap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    pipeline::Texture texture;
    texture.image = boost::make_shared<Image>(device_, spec);
    texture.sampler = sampler(nearest);
    upload(staging, *texture.image, VkExtent3D{source.width, source.height, 1});
    return texture;
}

/**
 **/
bool TextureFactory::fits(const Image& image, const Texels& source, uint32_t x, uint32_t y) {
    const Image::Spec& spec = image.spec();
    const bool floats = spec.format == VK_FORMAT_R32G32B32A32_SFLOAT && spec.depth <= 1 && (spec.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0;
    return floats && inside(image, x, y, source.width, source.height) && source.data != nullptr;
}

/**
 **/
void TextureFactory::update(const Image& image, const Texels& source, uint32_t x, uint32_t y) const {
    if (!fits(image, source, x, y)) {
        throw std::runtime_error("The texels do not fit the texture they were to be copied into");
    }
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(source.width) * source.height * FLOAT_TEXEL_BYTES;
    const boost::shared_ptr<Buffer> staging = boost::make_shared<Buffer>(device_,
        BufferSpec{.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .bytes = bytes});
    staging->write(source.data, bytes);
    copyRegion(staging, image, VkOffset3D{static_cast<int32_t>(x), static_cast<int32_t>(y), 0}, VkExtent3D{source.width, source.height, 1});
}

/**
 **/
void TextureFactory::update(const Image& image, const Pixels& source, uint32_t x, uint32_t y) const {
    if (!fits(image, source, x, y)) {
        throw std::runtime_error("The pixels do not fit the texture they were to be copied into");
    }
    copyRegion(stage(source), image, VkOffset3D{static_cast<int32_t>(x), static_cast<int32_t>(y), 0}, VkExtent3D{source.width, source.height, 1});
}

/**
 **/
void TextureFactory::copyRegion(const boost::shared_ptr<Buffer>& staging, const Image& image, const VkOffset3D& offset, const VkExtent3D& extent) const {
    VkBuffer from = staging->handle();
    VkImage target = image.handle();
    // the uploader holds the staging buffer until the copy has run
    uploader_->submit([from, target, offset, extent](VkCommandBuffer commands) {
        record(commands, {forRegionUpload(target)});

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageOffset = offset;
        copy.imageExtent = extent;
        vkCmdCopyBufferToImage(commands, from, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        record(commands, {uploadedForSampling(target)});
    }, staging);
}

/**
 **/
boost::shared_ptr<Buffer> TextureFactory::stage(const Pixels& source) const {
    const uint32_t uploaded = source.channels == 1 ? 1 : 4;
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(source.width) * source.height * uploaded;
    const boost::shared_ptr<Buffer> staging = boost::make_shared<Buffer>(device_,
        BufferSpec{.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .bytes = bytes});
    if (source.channels == 3) {
        // no device has to support a three channel format, so widen on the way in
        std::vector<unsigned char> widened(static_cast<std::size_t>(bytes));
        const std::size_t count = static_cast<std::size_t>(source.width) * source.height;
        for (std::size_t pixel = 0; pixel < count; pixel++) {
            widened[(pixel * 4) + 0] = source.data[(pixel * 3) + 0];
            widened[(pixel * 4) + 1] = source.data[(pixel * 3) + 1];
            widened[(pixel * 4) + 2] = source.data[(pixel * 3) + 2];
            widened[(pixel * 4) + 3] = 0xFF;
        }
        staging->write(widened.data(), bytes);
    } else {
        staging->write(source.data, bytes);
    }
    return staging;
}

/**
 **/
void TextureFactory::upload(const boost::shared_ptr<Buffer>& staging, const Image& image, const VkExtent3D& extent) const {
    VkBuffer source = staging->handle();
    VkImage target = image.handle();
    // the uploader holds the staging buffer until the copy has run
    uploader_->submit([source, target, extent](VkCommandBuffer commands) {
        record(commands, {forUpload(target)});

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = extent;
        vkCmdCopyBufferToImage(commands, source, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        record(commands, {uploadedForSampling(target)});
    }, staging);
}

};  // namespace v3d::render::realtime::vulkan::memory
