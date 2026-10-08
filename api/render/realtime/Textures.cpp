/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Textures.h"

#include <api/render/realtime/vulkan/frame/RenderTarget.h>

#include <map>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

namespace {

/**
 * How many sets a descriptor pool is created with. One set per texture; another pool is
 * added when this one is full.
 **/
const uint32_t poolSize = 64;

};  // namespace

/**
 **/
Textures::Textures(const boost::shared_ptr<vulkan::device::Device>& device,
    const boost::shared_ptr<vulkan::pipeline::Resources>& resources,
    const boost::shared_ptr<vulkan::frame::Ring>& ring,
    const boost::shared_ptr<vulkan::memory::Uploader>& uploader) :
    device_(device),
    resources_(resources) {
    factory_ = boost::make_shared<vulkan::memory::TextureFactory>(device_, uploader);

    VkDescriptorSetLayoutBinding sampler{};
    sampler.binding = 0;
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler.descriptorCount = 1;
    sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sets_ = boost::make_shared<vulkan::pipeline::DescriptorPool>(device_, ring,
        std::vector<VkDescriptorSetLayoutBinding>{sampler}, poolSize, "per material");

    const unsigned char pixel[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    white_ = texture(pixel, 1, 1, 4);
}

/**
 **/
TextureHandle Textures::texture(const boost::shared_ptr<v3d::image::Image>& image,
    vulkan::memory::TextureFactory::Encoding encoding, const vulkan::pipeline::Sampler::Spec& sampler) {
    return resources_->add(factory_->create(image, encoding, sampler));
}

/**
 **/
TextureHandle Textures::texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels,
    const vulkan::pipeline::Sampler::Spec& sampler) {
    return resources_->add(factory_->create(pixels, width, height, channels,
        vulkan::memory::TextureFactory::Encoding::Display, sampler));
}

/**
 **/
TextureHandle Textures::texture(const vulkan::frame::RenderTarget& target, uint32_t slot) {
    // a depth-only target has no colour to read, and a set written against no image is a
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
    const TextureHandle handle = resources_->add(texture);
    targets_[view] = handle;
    return handle;
}

/**
 **/
TextureHandle Textures::white() const noexcept {
    return white_;
}

/**
 **/
VkDescriptorSetLayout Textures::layout() const noexcept {
    return sets_->layout();
}

/**
 **/
MaterialHandle Textures::material(const TextureHandle& handle) {
    const std::map<TextureHandle, MaterialHandle>::const_iterator found = materials_.find(handle);
    if (found != materials_.end()) {
        return found->second;
    }

    const vulkan::pipeline::Texture* texture = resources_->texture(handle);
    if (texture == nullptr) {
        return MaterialHandle();
    }

    // a set that was released before is written again here, so every write is a full one
    VkDescriptorSet set = sets_->allocate();

    const VkDescriptorImageInfo image = texture->descriptor();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;

    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);

    vulkan::pipeline::Material built;
    built.set = set;
    built.texture = handle;

    const MaterialHandle material = resources_->add(built);
    materials_[handle] = material;
    return material;
}

/**
 **/
bool Textures::release(const TextureHandle& handle) {
    if (handle == white_) {
        return false;
    }

    const std::map<TextureHandle, MaterialHandle>::const_iterator found = materials_.find(handle);
    if (found != materials_.end()) {
        // a released material goes on resolving for the frame already queued, so only a
        // release that took effect hands the set back, and a set is never handed back twice
        const vulkan::pipeline::Material* material = resources_->material(found->second);
        VkDescriptorSet set = material != nullptr ? material->set : VK_NULL_HANDLE;
        if (resources_->release(found->second)) {
            sets_->release(set);
        }
        materials_.erase(found);
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
