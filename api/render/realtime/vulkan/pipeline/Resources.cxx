/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Resources.h"

#include <optional>

namespace v3d::render::realtime::vulkan::pipeline {

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
    // of its own to destroy, and a texture is destroyed by its registry going
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
    ring_->retire([texture = *released]() mutable { texture = Texture(); });
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
