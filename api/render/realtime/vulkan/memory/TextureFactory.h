/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "Uploader.h"

#include <boost/shared_ptr.hpp>

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime::vulkan::memory {

class Buffer;

/**
 * Turns pixels into a sampled image the shaders can read.
 *
 * Every texture is uploaded through a staging buffer and the Uploader, which submits the copy
 * and returns without waiting. The copy ends in a barrier that leaves the image ready for a
 * fragment shader to sample in any later submission. A new image has no frame reading it. A
 * region update copies into an image frames may be sampling, so its copy first waits on the
 * device for the fragment shaders of the frames submitted before it.
 *
 * A single channel image is given a view that swizzles its one channel into alpha and
 * ones into rgb. A coverage mask then samples as white with its coverage in alpha. A glyph
 * atlas holds signed distances, which the quad shader reads from alpha and thresholds.
 *
 * A texture is read through the sampler its spec names. The factory makes one sampler for each
 * distinct spec and shares it, so a scene of tiling textures holds one repeating sampler. Every
 * texture created with the default spec shares the factory's default sampler.
 **/
class TextureFactory final {
 public:
    /**
     * How the bytes of a colour image are read back by a shader.
     **/
    enum class Encoding : std::uint8_t {
        /**< as they were authored, as every texture drawn unlit is **/
        Display,
        /**< decoded from sRGB to linear when sampled, as a lit albedo is **/
        Srgb
    };

    /**
     * @param device the device the images are created on
     * @param uploader the context's Uploader, which copies every texture
     **/
    TextureFactory(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader);

    /**
     **/
    ~TextureFactory();

    TextureFactory(const TextureFactory&) = delete;
    TextureFactory& operator=(const TextureFactory&) = delete;

    /**
     * Pixels in tightly packed rows of width * channels bytes. The width and height are in
     * pixels. The channels are 1 for a coverage mask, or 3 or 4 for colour. 3 is widened to 4,
     * because a three channel format is not something a device has to support.
     **/
    struct Pixels {
        const unsigned char* data = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 4;
    };

    /**
     * @param source what to upload
     * @param encoding how a shader reads the colour back. A coverage mask is not a colour
     *        and ignores it
     * @param sampler how a shader reads the image. The default is linear and clamped to the
     *        edge, which suits an atlas and a sprite. A surface meant to tile names a repeating
     *        address mode
     * @return the created image and the sampler it is read through, for the caller to register
     * @throw std::runtime_error if any part of the creation or upload fails
     **/
    pipeline::Texture create(const Pixels& source, Encoding encoding = Encoding::Display,
        const pipeline::Sampler::Spec& sampler = pipeline::Sampler::Spec()) const;

    /**
     * @param image the image to upload, whose bpp decides the channel count
     **/
    pipeline::Texture create(const boost::shared_ptr<v3d::image::Image>& image, Encoding encoding = Encoding::Display,
        const pipeline::Sampler::Spec& sampler = pipeline::Sampler::Spec()) const;

    /**
     * @return whether pixels can be copied into an image at a place: the image is a 2D colour
     *         texture this factory made, the pixels lie inside it, and they have its channels.
     *         1 channel fits a coverage mask, and 3 or 4 fit a colour texture
     **/
    static bool fits(const Image& image, const Pixels& source, uint32_t x, uint32_t y);

    /**
     * Copy pixels into a region of a texture this factory made, which frames may be sampling.
     *
     * The copy is submitted at once and returns without waiting. On the device it waits for the
     * frames submitted before it to finish sampling the texture, so those draw what it held
     * before. The frame being recorded and every later one draw the new pixels. The rest of the
     * texture is kept.
     *
     * @param x and y where the region's top left texel goes
     * @throw std::runtime_error if the pixels do not fit the image, as fits() decides, or the
     *        upload fails
     **/
    void update(const Image& image, const Pixels& source, uint32_t x, uint32_t y) const;

    /**
     * A 3D texture, read as it is stored, because it is a lookup table and not an authored
     * colour.
     * @param texels four bytes a texel, rows then slices
     * @throw std::runtime_error if there are no texels or any part of the upload fails
     **/
    pipeline::Texture volume(const unsigned char* texels, uint32_t width, uint32_t height, uint32_t depth) const;

 private:
    /**
     * Copy what is staged into the whole of an image, leaving it ready to sample.
     **/
    void upload(const boost::shared_ptr<Buffer>& staging, const Image& image, const VkExtent3D& extent) const;

    /**
     * @return a staging buffer holding pixels as the image takes them, with three channels
     *         widened to four
     **/
    boost::shared_ptr<Buffer> stage(const Pixels& source) const;

    /**
     * @return the sampler made for a spec, made now if no texture has named that spec before
     **/
    boost::shared_ptr<pipeline::Sampler> sampler(const pipeline::Sampler::Spec& spec) const;

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Uploader> uploader_;
    /**< one sampler per distinct spec, the default first. Mutable because create() is const:
     *   a new sampler changes only this cache, not what the factory makes **/
    mutable std::vector<std::pair<pipeline::Sampler::Spec, boost::shared_ptr<pipeline::Sampler>>> samplers_;
};

};  // namespace v3d::render::realtime::vulkan::memory
