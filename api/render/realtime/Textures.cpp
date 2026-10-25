/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Textures.h"

#include <api/image/Image.h>
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

#include <cstdint>
#include <map>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

/**
 **/
Textures::Textures(const boost::shared_ptr<vulkan::device::Device>& device,
    const boost::shared_ptr<vulkan::pipeline::Resources>& resources,
    const boost::shared_ptr<vulkan::frame::Ring>& ring,
    const boost::shared_ptr<vulkan::memory::Uploader>& uploader) :
    device_(device),
    resources_(resources) {
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
    const uint32_t slot = array_->add(texture.descriptor());
    TextureHandle handle;
    try {
        handle = resources_->add(texture);
    } catch (...) {
        array_->release(slot);
        throw;
    }
    slots_[handle] = slot;
    return handle;
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
