/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>

#include <vulkan/vulkan.h>

#include <cstdint>

#include "Uploader.h"

#include <boost/shared_ptr.hpp>

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime::vulkan::memory {

/**
 * Turns pixels into a sampled image the shaders can read.
 *
 * Every texture is uploaded through a staging buffer and a one-shot command buffer that
 * the factory waits on before returning - see Uploader. Textures are built at load time,
 * so nothing needs an upload that overlaps the frames being drawn.
 *
 * A single channel image is given a view that swizzles its one channel into alpha and
 * ones into rgb, so a glyph atlas samples as white-with-coverage and the one quad shader
 * serves both text and sprites without a branch - see ADR-0005.
 *
 * Every texture is read through the same sampler, which the factory makes once and each
 * texture shares.
 **/
class TextureFactory final {
 public:
    /**
     * How the bytes of a colour image are read back by a shader.
     **/
    enum class Encoding {
        /**< as they were authored, which is every texture drawn unlit - ADR-0009 **/
        Display,
        /**< decoded from sRGB to linear when sampled, which a lit albedo is - ADR-0066 **/
        Srgb
    };

    /**
     * @param device the device the images are created on
     **/
    explicit TextureFactory(const boost::shared_ptr<device::Device>& device);

    /**
     **/
    ~TextureFactory();

    TextureFactory(const TextureFactory&) = delete;
    TextureFactory& operator=(const TextureFactory&) = delete;

    /**
     * @param pixels tightly packed rows of width * channels bytes
     * @param width in pixels
     * @param height in pixels
     * @param channels 1 for a coverage mask, 3 or 4 for colour - 3 is widened to 4,
     *        because a three channel format is not something a device has to support
     * @param encoding how a shader reads the colour back. A coverage mask is not a colour
     *        and ignores it
     * @return the created image and the sampler it is read through, for the caller to register
     * @throw std::runtime_error if any part of the creation or upload fails
     **/
    pipeline::Texture create(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels,
        Encoding encoding = Encoding::Display) const;

    /**
     * @param image the image to upload, whose bpp decides the channel count
     **/
    pipeline::Texture create(const boost::shared_ptr<v3d::image::Image>& image, Encoding encoding = Encoding::Display) const;

 private:
    /**
     * Move the image between layouts either side of the copy.
     **/
    static void transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to);

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Uploader> uploader_;
    boost::shared_ptr<pipeline::Sampler> sampler_;
};

};  // namespace v3d::render::realtime::vulkan::memory
