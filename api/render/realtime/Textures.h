/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <map>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

namespace vulkan::frame {
class RenderTarget;
};  // namespace vulkan::frame

/**
 * The textures a context holds and the materials they are sampled through.
 *
 * Every pipeline that samples a texture at set 1 declares this one layout, so a material made
 * here serves the quad, the world quad and the lit renderer alike and an atlas is uploaded
 * once. It belongs to the context rather than to any renderer, so a context that draws no
 * quads - a headless one, or one loading meshes - registers textures all the same.
 *
 * What is registered belongs to pipeline::Resources. The white texture lives as long as the
 * context; a texture lives until it is released here, along with its material.
 **/
class Textures final {
 public:
    /**
     * @param uploader the context's one-shot queue, which every texture is copied through
     * @throw std::runtime_error if the white texture cannot be uploaded
     **/
    Textures(const boost::shared_ptr<vulkan::device::Device>& device,
        const boost::shared_ptr<vulkan::pipeline::Resources>& resources,
        const boost::shared_ptr<vulkan::frame::Ring>& ring,
        const boost::shared_ptr<vulkan::memory::Uploader>& uploader);

    Textures(const Textures&) = delete;
    Textures& operator=(const Textures&) = delete;

    /**
     * Upload an image and register it, so a draw can name it.
     * @param encoding how a shader reads it back - as authored unless something lights it
     **/
    TextureHandle texture(const boost::shared_ptr<v3d::image::Image>& image,
        vulkan::memory::TextureFactory::Encoding encoding = vulkan::memory::TextureFactory::Encoding::Display);

    /**
     * @param pixels tightly packed rows of width * channels bytes
     * @param channels 1 for a coverage mask such as a glyph atlas, 3 or 4 for colour
     **/
    TextureHandle texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels);

    /**
     * Register a render target so that a draw can sample what a pass drew into it.
     *
     * What is registered shares the target's images. Registering the same image again gives
     * back the handle it already has, so calling this every frame costs nothing. A target that
     * is resized allocates new images, and the handle this returned goes on naming the old
     * ones, which it keeps alive: after a recreate(), release the old handle and register the
     * target again.
     *
     * A target with no colour image has nothing to register, and comes back as the white
     * texture for the same reason depthTexture() gives one for a target with no depth to read.
     *
     * @param slot which of the target's images - one handle per slot for a target holding one
     *        per frame in flight, of which a reader names current() or previous() each frame
     **/
    TextureHandle texture(const vulkan::frame::RenderTarget& target, uint32_t slot = 0);

    /**
     * Register a render target's depth image, so that a draw can sample what a pass tested
     * against rather than what it painted. A shadow map is read this way.
     *
     * The registration shares the target's image, and the same rule applies about releasing
     * and registering again after a recreate(). A target built without a depth image, or
     * with one not created as sampled, has nothing to register. It comes back as the white
     * texture, because a set written against an image with no sampled usage is undefined,
     * and a white shadow map only leaves the scene unshadowed.
     **/
    TextureHandle depthTexture(const vulkan::frame::RenderTarget& target, uint32_t slot = 0);

    /**
     * @return the 1x1 white texture an untextured draw samples
     **/
    TextureHandle white() const noexcept;

    /**
     * The descriptor set that binds a texture at set 1, created on first use and kept.
     *
     * @return the material to name on a draw item, or an unset handle for a texture this does
     *         not hold
     **/
    MaterialHandle material(const TextureHandle& handle);

    /**
     * @return set 1's layout, which every pipeline sampling a texture declares
     **/
    VkDescriptorSetLayout layout() const noexcept;

    /**
     * Release a texture and the material drawn with it. The handle resolves to
     * nothing at once, and the image and the descriptor set are reclaimed once no frame in
     * flight can still be reading them.
     *
     * @return whether anything was released. The white texture is never released, so a handle
     *         depthTexture() gave back for a target with nothing to sample can be released
     *         like any other without taking it away from everything else.
     **/
    bool release(const TextureHandle& handle);

 private:
    boost::shared_ptr<vulkan::device::Device> device_;
    boost::shared_ptr<vulkan::pipeline::Resources> resources_;
    boost::shared_ptr<vulkan::memory::TextureFactory> factory_;
    boost::shared_ptr<vulkan::pipeline::DescriptorPool> sets_;  /**< set 1, the sampler every textured draw reads through **/
    TextureHandle white_;
    /**< keyed by the whole handle, so a slot reused after a release never finds the old set **/
    std::map<TextureHandle, MaterialHandle> materials_;
    /**
     * The handle each registered render target image has, by its view. A view cannot be reused
     * while its entry is here, because the registered texture keeps the image alive until it is
     * released, and a release removes the entry.
     **/
    std::map<VkImageView, TextureHandle> targets_;

    /** A render target image's handle, registered the first time it is asked for. **/
    TextureHandle registered(const vulkan::pipeline::Texture& texture);
};

};  // namespace v3d::render::realtime
