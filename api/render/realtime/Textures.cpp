/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Textures.h"

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/asset/media/kind/Image.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>
#include <api/render/realtime/vulkan/pipeline/Material.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>
#include <api/render/realtime/vulkan/pipeline/TextureArray.h>

#include "Handle.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::render::realtime {

namespace {

/**
 * What an exception says, for the line a failed request is reported on.
 **/
std::string describe(const std::exception_ptr& error) {
    if (!error) {
        return "the file holds no image";
    }
    try {
        std::rethrow_exception(error);
    } catch (const std::exception& thrown) {
        return thrown.what();
    } catch (...) {
        return "an unknown error";
    }
}

};  // namespace

/**
 **/
Textures::Textures(const boost::shared_ptr<vulkan::device::Device>& device,
    const boost::shared_ptr<vulkan::pipeline::Resources>& resources,
    const boost::shared_ptr<vulkan::frame::Ring>& ring,
    const boost::shared_ptr<vulkan::memory::Uploader>& uploader) :
    device_(device),
    resources_(resources),
    ring_(ring),
    inbox_(boost::make_shared<Inbox>()) {
    factory_ = boost::make_shared<vulkan::memory::TextureFactory>(device_, uploader);
    array_ = boost::make_shared<vulkan::pipeline::TextureArray>(device_, ring);

    vulkan::pipeline::Material array;
    array.set = array_->set();
    material_ = resources_->add(array);

    const unsigned char pixel[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    white_ = texture(pixel, 1, 1, 4);
}

/**
 **/
TextureHandle Textures::texture(const boost::shared_ptr<v3d::image::Image>& image,
    vulkan::memory::TextureFactory::Encoding encoding, const vulkan::pipeline::Sampler::Spec& sampler) {
    return add(factory_->create(image, encoding, sampler));
}

/**
 **/
TextureHandle Textures::texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels,
    const vulkan::pipeline::Sampler::Spec& sampler) {
    return add(factory_->create({.data = pixels, .width = width, .height = height, .channels = channels},
        vulkan::memory::TextureFactory::Encoding::Display, sampler));
}

/**
 **/
void Textures::requestFrom(const boost::shared_ptr<asset::Manager>& assets, const boost::shared_ptr<asset::Worker>& worker,
    const boost::shared_ptr<log::Logger>& logger) {
    assets_ = assets;
    worker_ = worker;
    logger_ = logger;
}

/**
 **/
TextureHandle Textures::request(const std::string& path, vulkan::memory::TextureFactory::Encoding encoding,
    const vulkan::pipeline::Sampler::Spec& sampler) {
    if (!assets_ || !worker_) {
        throw std::logic_error("Textures has no asset manager and worker to request " + path + " from");
    }
    const std::map<std::string, TextureHandle>::const_iterator known = requested_.find(path);
    if (known != requested_.end()) {
        return known->second;
    }

    // until the image arrives the handle names the white texture's image, and has no slot of its
    // own, so slot() gives the white texture's
    const TextureHandle handle = resources_->add(*resources_->texture(white_));
    requests_[handle] = Request{.path = path, .encoding = encoding, .sampler = sampler};
    requested_[path] = handle;
    try {
        worker_->post([assets = assets_, path]() {
                const boost::shared_ptr<asset::media::kind::Image> image = assets->load<asset::media::kind::Image>(path);
                return image ? image->image() : boost::shared_ptr<image::Image>();
            },
            [inbox = boost::weak_ptr<Inbox>(inbox_), handle](std::expected<boost::shared_ptr<image::Image>, std::exception_ptr> result) {
                const boost::shared_ptr<Inbox> held = inbox.lock();
                if (held) {
                    held->push_back(Arrival{handle, std::move(result)});
                }
            });
    } catch (...) {
        release(handle);
        throw;
    }
    return handle;
}

/**
 **/
std::size_t Textures::poll(std::size_t budget) {
    std::size_t count = 0;
    std::size_t spent = 0;
    while (!inbox_->empty() && (count == 0 || spent < budget)) {
        const Arrival arrival = std::move(inbox_->front());
        inbox_->pop_front();

        // a request released before its image arrived has nothing left to fill
        const std::map<TextureHandle, Request>::iterator found = requests_.find(arrival.handle);
        if (found == requests_.end() || !found->second.pending) {
            continue;
        }
        Request& request = found->second;
        request.pending = false;
        if (!arrival.result || !*arrival.result) {
            if (logger_) {
                logger_->get()->error("The texture {} could not be loaded, and is drawn white: {}", request.path,
                    describe(arrival.result ? std::exception_ptr() : arrival.result.error()));
            }
            continue;
        }
        try {
            spent += fill(arrival, &request);
        } catch (const std::exception& error) {
            if (logger_) {
                logger_->get()->error("The texture {} could not be uploaded, and is drawn white: {}", request.path, error.what());
            }
            continue;
        }
        count++;
    }
    return count;
}

/**
 **/
std::size_t Textures::fill(const Arrival& arrival, Request* request) {
    const boost::shared_ptr<image::Image>& image = *arrival.result;
    const vulkan::pipeline::Texture texture = factory_->create(image, request->encoding, request->sampler);
    const uint32_t slot = place(texture);
    // items queued before now were given the white texture's slot and draw white. Every one
    // submitted after reads the texture's own
    resources_->replace(arrival.handle, texture);
    slots_[arrival.handle] = slot;
    return static_cast<std::size_t>(image->width()) * image->height() * 4;
}

/**
 **/
TextureHandle Textures::texture(const vulkan::frame::RenderTarget& target, uint32_t slot) {
    // a depth-only target has no colour to read, and a slot written against no image is a
    // validation error. It gets the white texture, as depthTexture() does for a target with
    // no depth to read
    if (target.view() == VK_NULL_HANDLE || slot >= target.images()) {
        return white_;
    }
    return registered(target.texture(slot));
}

/**
 **/
TextureHandle Textures::depthTexture(const vulkan::frame::RenderTarget& target, uint32_t slot) {
    if (!target.sampledDepth() || slot >= target.images()) {
        return white_;
    }
    return registered(target.depthTexture(slot));
}

/**
 **/
TextureHandle Textures::registered(const vulkan::pipeline::Texture& texture) {
    VkImageView view = texture.image->view();
    const std::map<VkImageView, TextureHandle>::const_iterator found = targets_.find(view);
    if (found != targets_.end()) {
        return found->second;
    }
    const TextureHandle handle = add(texture);
    targets_[view] = handle;
    return handle;
}

/**
 **/
TextureHandle Textures::add(const vulkan::pipeline::Texture& texture) {
    const uint32_t slot = place(texture);
    TextureHandle handle;
    try {
        handle = resources_->add(texture);
    } catch (...) {
        array_->release(slot);
        ring_->retire([texture]() {});
        throw;
    }
    slots_[handle] = slot;
    return handle;
}

/**
 **/
uint32_t Textures::place(const vulkan::pipeline::Texture& texture) {
    try {
        return array_->add(texture.descriptor());
    } catch (...) {
        ring_->retire([texture]() {});
        throw;
    }
}

/**
 **/
TextureHandle Textures::white() const noexcept {
    return white_;
}

/**
 **/
MaterialHandle Textures::material() const noexcept {
    return material_;
}

/**
 **/
uint32_t Textures::slot(const TextureHandle& handle) const {
    std::map<TextureHandle, uint32_t>::const_iterator found = slots_.find(handle);
    if (found == slots_.end()) {
        found = slots_.find(white_);
    }
    return found != slots_.end() ? found->second : 0;
}

/**
 **/
VkDescriptorSetLayout Textures::layout() const noexcept {
    return array_->layout();
}

/**
 **/
uint32_t Textures::capacity() const noexcept {
    return array_->capacity();
}

/**
 **/
bool Textures::release(const TextureHandle& handle) {
    if (handle == white_) {
        return false;
    }

    const std::map<TextureHandle, Request>::const_iterator request = requests_.find(handle);
    if (request != requests_.end()) {
        // what the worker decodes for a request still pending finds nothing to fill
        requested_.erase(request->second.path);
        requests_.erase(request);
    }

    const std::map<TextureHandle, uint32_t>::const_iterator found = slots_.find(handle);
    if (found != slots_.end()) {
        // items queued before the release were given this slot, so it is freed only once the
        // frame being queued has finished
        array_->release(found->second);
        slots_.erase(found);
    }
    for (std::map<VkImageView, TextureHandle>::const_iterator target = targets_.begin(); target != targets_.end(); ++target) {
        if (target->second == handle) {
            targets_.erase(target);
            break;
        }
    }

    return resources_->release(handle);
}

};  // namespace v3d::render::realtime
