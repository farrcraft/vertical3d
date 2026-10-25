/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/pipeline/TextureArray.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <expected>
#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

namespace vulkan::frame {
class RenderTarget;
};  // namespace vulkan::frame

/**
 * The textures a context holds, each in a slot of one texture array.
 *
 * Every pipeline that samples a texture declares the array's layout at set 1 and binds its one
 * material, which is the same for every draw. A draw names its texture by slot(), read from the
 * handle when the draw is submitted, so the quad, the world quad and the lit renderer share
 * every texture, and an atlas is uploaded once. It belongs to the context, not to any renderer,
 * so a context that draws no quads, such as a headless one or one loading meshes, can still
 * register textures.
 *
 * What is registered belongs to pipeline::Resources. The white texture lives as long as the
 * context. Any other texture lives until it is released here, along with its slot.
 **/
class Textures final {
 public:
    /**
     * The most bytes poll() uploads by default before it leaves the rest for the next call.
     * MeshRegistry's poll() takes the same.
     **/
    static constexpr std::size_t DEFAULT_UPLOAD_BUDGET = std::size_t{4} * 1024 * 1024;

    /**
     * @param uploader the context's Uploader, which copies every texture
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
     * @param encoding how a shader reads it back: as authored, unless the texture is lit
     * @param sampler how a shader reads the image. The default is linear and clamped to the
     *        edge. A surface meant to tile names a repeating address mode
     **/
    TextureHandle texture(const boost::shared_ptr<v3d::image::Image>& image,
        vulkan::memory::TextureFactory::Encoding encoding = vulkan::memory::TextureFactory::Encoding::Display,
        const vulkan::pipeline::Sampler::Spec& sampler = vulkan::pipeline::Sampler::Spec());

    /**
     * @param pixels tightly packed rows of width * channels bytes
     * @param channels 1 for a coverage mask or a glyph atlas, 3 or 4 for colour
     * @param sampler how a shader reads the image, as above
     **/
    TextureHandle texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels,
        const vulkan::pipeline::Sampler::Spec& sampler = vulkan::pipeline::Sampler::Spec());

    /**
     * Where request() reads an image from and decodes it: the asset manager a path is relative
     * to, and the worker it is decoded on. Until this is called, request() throws.
     *
     * @param logger where an image that could not be read or uploaded is reported
     **/
    void requestFrom(const boost::shared_ptr<asset::Manager>& assets, const boost::shared_ptr<asset::Worker>& worker,
        const boost::shared_ptr<log::Logger>& logger);

    /**
     * An image read and decoded on the worker, and uploaded by a later poll().
     *
     * The handle can be drawn at once. Until poll() uploads the image it reads the white
     * texture's slot, and resolves to a texture holding the white image. Asking for a path again
     * returns the same handle, uploaded or not. An image that cannot be read or uploaded is
     * reported in the log, and its handle reads white until it is released. Releasing a request
     * before its image arrives drops what the worker decodes for it.
     *
     * @param path relative to the asset manager's root
     * @param encoding and sampler as texture() takes them
     * @throw std::logic_error if requestFrom() has not been called
     **/
    TextureHandle request(const std::string& path,
        vulkan::memory::TextureFactory::Encoding encoding = vulkan::memory::TextureFactory::Encoding::Display,
        const vulkan::pipeline::Sampler::Spec& sampler = vulkan::pipeline::Sampler::Spec());

    /**
     * Upload the requested images whose decoding has finished, in the order they were requested.
     *
     * It uploads at least one when any is ready, and stops once the bytes uploaded reach the
     * budget. Call it once a frame, after polling the worker. A handle uploaded here reads its
     * own slot from the next draw that is submitted.
     *
     * @param budget the bytes of texels to upload before stopping
     * @return how many images were uploaded
     **/
    std::size_t poll(std::size_t budget = DEFAULT_UPLOAD_BUDGET);

    /**
     * Register a render target so that a draw can sample what a pass drew into it.
     *
     * What is registered shares the target's images. Registering the same image again gives
     * back the handle it already has, so calling this every frame costs nothing. The handle is
     * shared by everything that registered that image, and releasing it releases it for all
     * of them. A target that is resized allocates new images, and the handle this returned
     * goes on naming the old ones, which it keeps alive: after a recreate(), release the old
     * handle and register the target again.
     *
     * A target with no colour image has nothing to register, and returns the white texture,
     * as depthTexture() does for a target with no depth to read.
     *
     * @param slot which of the target's images. A target with one image per frame in flight
     *        has one handle per slot, and a reader names current() or previous() each frame.
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
     * @return the material that binds the texture array at set 1, the same for every textured
     *         draw. It lives as long as the context
     **/
    MaterialHandle material() const noexcept;

    /**
     * Where a texture sits in the array, for a draw to read it by.
     *
     * Read it when the draw is submitted, not once and kept, because a texture filled or
     * replaced later moves to another slot.
     *
     * @return the texture's slot, or the white texture's for an unset handle or one released
     **/
    uint32_t slot(const TextureHandle& handle) const;

    /**
     * @return set 1's layout, which every pipeline sampling a texture declares
     **/
    VkDescriptorSetLayout layout() const noexcept;

    /**
     * @return how many textures the array can hold at once, the white texture among them
     **/
    uint32_t capacity() const noexcept;

    /**
     * Release a texture and its slot.
     *
     * The texture handle resolves to nothing at once, and slot() gives the white texture's.
     * Items queued before the release keep the slot they were given, and draw with the texture.
     * The image and the slot are reclaimed once the frame being queued has finished.
     *
     * @return whether anything was released. The white texture is never released, so a handle
     *         depthTexture() gave back for a target with nothing to sample can be released
     *         like any other without taking it away from everything else.
     **/
    bool release(const TextureHandle& handle);

 private:
    boost::shared_ptr<vulkan::device::Device> device_;
    boost::shared_ptr<vulkan::pipeline::Resources> resources_;
    boost::shared_ptr<vulkan::frame::Ring> ring_;
    boost::shared_ptr<vulkan::memory::TextureFactory> factory_;
    boost::shared_ptr<vulkan::pipeline::TextureArray> array_;  /**< set 1, which every textured draw reads through **/
    MaterialHandle material_;  /**< what binds array_ **/
    TextureHandle white_;
    /**< keyed by the whole handle, so a handle's slot reused after a release never finds the old slot **/
    std::map<TextureHandle, uint32_t> slots_;
    /**
     * The handle each registered render target image has, by its view. A view cannot be reused
     * while its entry is here, because the registered texture keeps the image alive until it is
     * released, and a release removes the entry.
     **/
    std::map<VkImageView, TextureHandle> targets_;

    /** A render target image's handle, registered the first time it is asked for. **/
    TextureHandle registered(const vulkan::pipeline::Texture& texture);

    /** Register a texture, and give it a slot. **/
    TextureHandle add(const vulkan::pipeline::Texture& texture);

    /**
     * A slot for a texture just made. A texture that has none is retired through the ring, not
     * destroyed, because its upload may still be running.
     *
     * @throw std::runtime_error if every slot is taken
     **/
    uint32_t place(const vulkan::pipeline::Texture& texture);

    /**
     * What a request() asked for, kept while its handle is held. It is pending until its image
     * is uploaded or fails.
     **/
    struct Request final {
        std::string path;
        vulkan::memory::TextureFactory::Encoding encoding;
        vulkan::pipeline::Sampler::Spec sampler;
        bool pending = true;
    };

    /**
     * An image the worker has finished with, waiting for poll(): the image, null for a file
     * that held none, or what reading it threw.
     **/
    struct Arrival final {
        TextureHandle handle;
        std::expected<boost::shared_ptr<image::Image>, std::exception_ptr> result;
    };
    using Inbox = std::deque<Arrival>;

    /** Upload an arrival into its request's handle, and say how many bytes it cost. **/
    std::size_t fill(const Arrival& arrival, Request* request);

    boost::shared_ptr<asset::Manager> assets_;
    boost::shared_ptr<asset::Worker> worker_;
    boost::shared_ptr<log::Logger> logger_;
    /** Held by every completion as a weak pointer, so one that runs after the Textures is gone drops its image. **/
    boost::shared_ptr<Inbox> inbox_;
    std::map<TextureHandle, Request> requests_;
    /** The handle each requested path was given. **/
    std::map<std::string, TextureHandle> requested_;
};

};  // namespace v3d::render::realtime
