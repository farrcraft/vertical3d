/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Resources.h"

#include <optional>

namespace v3d::render::realtime::vulkan::pipeline {

namespace {

/**
 * Destroy what a texture owns, in the order vulkan requires.
 **/
void destroy(const boost::shared_ptr<device::Device>& device, Texture texture) {
    // a texture that only names someone else's images - a render target's - is a reference
    // rather than a resource, and freeing it here would free it twice
    if (!texture.owned) {
        return;
    }
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device->handle(), texture.sampler, nullptr);
    }
    if (texture.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device->handle(), texture.view, nullptr);
    }
    if (texture.image != VK_NULL_HANDLE) {
        vkDestroyImage(device->handle(), texture.image, nullptr);
    }
    // the memory outlives the image it backs, so it goes last
    device->allocator().free(&texture.memory);
}

};  // namespace

/**
 **/
Pipeline::Pipeline() noexcept :
    pipeline(VK_NULL_HANDLE),
    layout(VK_NULL_HANDLE),
    pushStages(0) {
}

/**
 **/
Material::Material() noexcept :
    set(VK_NULL_HANDLE) {
}

/**
 **/
Texture::Texture() noexcept :
    image(VK_NULL_HANDLE),
    view(VK_NULL_HANDLE),
    sampler(VK_NULL_HANDLE),
    extent(),
    owned(true) {
}

/**
 **/
Resources::Resources(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring) :
    device_(device),
    ring_(ring) {
}

/**
 **/
Resources::~Resources() {
    VkDevice device = device_->handle();

    pipelines_.each([device](const Pipeline& pipeline) {
        if (pipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline.pipeline, nullptr);
        }
        if (pipeline.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
        }
    });

    // descriptor sets are freed with the pool they came from, so a material owns nothing
    // of its own to destroy

    textures_.each([this](const Texture& texture) { destroy(device_, texture); });
}

/**
 **/
PipelineHandle Resources::add(const Pipeline& pipeline) {
    return pipelines_.add(pipeline);
}

/**
 **/
MaterialHandle Resources::add(const Material& material) {
    return materials_.add(material);
}

/**
 **/
TextureHandle Resources::add(const Texture& texture) {
    return textures_.add(texture);
}

/**
 **/
bool Resources::release(const MaterialHandle& handle) {
    return materials_.release(handle).has_value();
}

/**
 **/
bool Resources::release(const TextureHandle& handle) {
    std::optional<Texture> released = textures_.release(handle);
    if (!released) {
        return false;
    }
    ring_->retire([device = device_, texture = *released]() { destroy(device, texture); });
    return true;
}

/**
 **/
const Pipeline* Resources::pipeline(const PipelineHandle& handle) const {
    return pipelines_.resolve(handle);
}

/**
 **/
const Material* Resources::material(const MaterialHandle& handle) const {
    return materials_.resolve(handle);
}

/**
 **/
const Texture* Resources::texture(const TextureHandle& handle) const {
    return textures_.resolve(handle);
}

};  // namespace v3d::render::realtime::vulkan::pipeline
