/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Resources.h"

namespace v3d::render::realtime::vulkan {

    /**
     **/
    Pipeline::Pipeline() noexcept :
        pipeline(VK_NULL_HANDLE),
        layout(VK_NULL_HANDLE) {
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
        memory(VK_NULL_HANDLE),
        view(VK_NULL_HANDLE),
        sampler(VK_NULL_HANDLE),
        extent{0, 0} {
    }

    /**
     **/
    Resources::Resources(const boost::shared_ptr<Device>& device) :
        device_(device) {
    }

    /**
     **/
    Resources::~Resources() {
        VkDevice device = device_->handle();

        for (const Pipeline& pipeline : pipelines_.resources()) {
            if (pipeline.pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pipeline.pipeline, nullptr);
            }
            if (pipeline.layout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
            }
        }

        // descriptor sets are freed with the pool they came from, so a material owns nothing
        // of its own to destroy

        for (const Texture& texture : textures_.resources()) {
            if (texture.sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device, texture.sampler, nullptr);
            }
            if (texture.view != VK_NULL_HANDLE) {
                vkDestroyImageView(device, texture.view, nullptr);
            }
            if (texture.image != VK_NULL_HANDLE) {
                vkDestroyImage(device, texture.image, nullptr);
            }
            // the memory outlives the image it backs, so it goes last
            if (texture.memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, texture.memory, nullptr);
            }
        }
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

};  // namespace v3d::render::realtime::vulkan
