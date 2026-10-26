/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Resources.h"

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/pipeline/Pipeline.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>

#include "Material.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

/**
 **/
Resources::Resources(const boost::shared_ptr<vulkan::device::Device>& device, const boost::shared_ptr<vulkan::frame::Ring>& ring) :
    device_(device),
    ring_(ring),
    held_(boost::make_shared<Held>()) {
}

/**
 **/
Resources::~Resources() {
    VkDevice device = device_->handle();

    pipelines_.each([device](const vulkan::pipeline::Pipeline& pipeline) {
        if (pipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline.pipeline, nullptr);
        }
        if (pipeline.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
        }
    });

    // descriptor sets are freed with the pool they came from, so a material owns nothing
    // of its own to destroy. A texture is destroyed with Held, which a release still waiting
    // in the Ring keeps alive until it runs
}

/**
 **/
PipelineHandle Resources::add(const vulkan::pipeline::Pipeline& pipeline) {
    return pipelines_.add(pipeline);
}

/**
 **/
MaterialHandle Resources::add(const Material& material) {
    return held_->materials.add(material);
}

/**
 **/
TextureHandle Resources::add(const vulkan::pipeline::Texture& texture) {
    return held_->textures.add(texture);
}

/**
 **/
bool Resources::release(const MaterialHandle& handle) {
    if (held_->materials.resolve(handle) == nullptr || held_->retiringMaterials.count(handle) > 0) {
        return false;
    }
    // marked before retire(), and unmarked if retire() throws, so a failed release leaves the
    // material registered as it was
    held_->retiringMaterials[handle] = ring_->recording();
    try {
        ring_->retire([held = held_, handle]() {
            held->retiringMaterials.erase(handle);
            held->materials.release(handle);
        });
    } catch (...) {
        held_->retiringMaterials.erase(handle);
        throw;
    }
    return true;
}

/**
 **/
bool Resources::release(const TextureHandle& handle) {
    if (held_->textures.resolve(handle) == nullptr || held_->retiringTextures.count(handle) > 0) {
        return false;
    }
    held_->retiringTextures.insert(handle);
    try {
        // the texture released from the registry is destroyed as the callback returns
        ring_->retire([held = held_, handle]() {
            held->retiringTextures.erase(handle);
            held->textures.release(handle);
        });
    } catch (...) {
        held_->retiringTextures.erase(handle);
        throw;
    }
    return true;
}

/**
 **/
bool Resources::replace(const TextureHandle& handle, const vulkan::pipeline::Texture& texture) {
    vulkan::pipeline::Texture* held = held_->textures.resolve(handle);
    if (held == nullptr || held_->retiringTextures.count(handle) > 0) {
        return false;
    }
    // the texture replaced is destroyed as the callback returns
    ring_->retire([replaced = *held]() {});
    *held = texture;
    return true;
}

/**
 **/
std::size_t Resources::textureCount() const noexcept {
    return held_->textures.count();
}

/**
 **/
const vulkan::pipeline::Pipeline* Resources::pipeline(const PipelineHandle& handle) const {
    return pipelines_.resolve(handle);
}

/**
 **/
const Material* Resources::material(const MaterialHandle& handle) const {
    const std::map<MaterialHandle, uint64_t>::const_iterator retiring = held_->retiringMaterials.find(handle);
    if (retiring != held_->retiringMaterials.end() && ring_->recording() > retiring->second) {
        return nullptr;
    }
    return held_->materials.resolve(handle);
}

/**
 **/
const vulkan::pipeline::Texture* Resources::texture(const TextureHandle& handle) const {
    if (held_->retiringTextures.count(handle) > 0) {
        return nullptr;
    }
    return held_->textures.resolve(handle);
}

};  // namespace v3d::render::realtime
