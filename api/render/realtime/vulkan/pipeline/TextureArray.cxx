/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureArray.h"

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Result.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

namespace {

/**
 * The most slots an array has. Each slot costs a descriptor in the one set, written once, so the
 * size bounds how many textures a context holds at once rather than any per-frame cost.
 **/
constexpr uint32_t mostSlots = 4096;

/**
 * Descriptors left over for the other sets a pipeline declares beside the array, such as the
 * lit scene's shadow map, under the device's per-stage limit.
 **/
constexpr uint32_t otherSamplers = 16;

/**
 * How many slots the device allows: the fewest of its per-stage and per-set limits on samplers
 * and sampled images, less what other sets may take, and no more than mostSlots.
 **/
uint32_t slots(VkPhysicalDevice physical) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical, &properties);
    const VkPhysicalDeviceLimits& limits = properties.limits;
    const uint32_t allowed = std::min({limits.maxPerStageDescriptorSamplers, limits.maxPerStageDescriptorSampledImages,
        limits.maxDescriptorSetSamplers, limits.maxDescriptorSetSampledImages});
    if (allowed <= otherSamplers) {
        throw std::runtime_error("The vulkan device allows too few samplers in a stage for a texture array");
    }
    return std::min(mostSlots, allowed - otherSamplers);
}

};  // namespace

/**
 **/
TextureArray::TextureArray(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring) :
    device_(device),
    ring_(ring),
    layout_(VK_NULL_HANDLE),
    pool_(VK_NULL_HANDLE),
    set_(VK_NULL_HANDLE),
    capacity_(slots(device->physical())),
    free_(boost::make_shared<std::vector<uint32_t>>()) {
    // a slot may be written while a frame that binds the set is pending, as long as that frame
    // does not read the slot, and a slot nothing has written is never read
    const VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
    bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlags.bindingCount = 1;
    bindingFlags.pBindingFlags = &flags;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = capacity_;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = &bindingFlags;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    device::check(vkCreateDescriptorSetLayout(device_->handle(), &layoutInfo, nullptr, &layout_),
        "Unable to create the texture array's descriptor set layout");

    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    size.descriptorCount = capacity_;
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &size;
    VkResult result = vkCreateDescriptorPool(device_->handle(), &poolInfo, nullptr, &pool_);
    if (result != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device_->handle(), layout_, nullptr);
        throw device::failure(result, "Unable to create the texture array's descriptor pool");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &layout_;
    result = vkAllocateDescriptorSets(device_->handle(), &allocateInfo, &set_);
    if (result != VK_SUCCESS) {
        vkDestroyDescriptorPool(device_->handle(), pool_, nullptr);
        vkDestroyDescriptorSetLayout(device_->handle(), layout_, nullptr);
        throw device::failure(result, "Unable to allocate the texture array's descriptor set");
    }

    // the lowest slots are handed out first
    free_->reserve(capacity_);
    for (uint32_t slot = capacity_; slot > 0; slot--) {
        free_->push_back(slot - 1);
    }
}

/**
 **/
TextureArray::~TextureArray() {
    const boost::shared_ptr<device::Device> device = device_;
    VkDescriptorSetLayout layout = layout_;
    VkDescriptorPool pool = pool_;
    const auto destroy = [device, layout, pool]() {
        vkDestroyDescriptorPool(device->handle(), pool, nullptr);
        vkDestroyDescriptorSetLayout(device->handle(), layout, nullptr);
    };
    // a frame already queued or in flight may bind the set. If retire() throws, the pool is
    // destroyed once the device is idle
    try {
        ring_->retire(destroy);
    } catch (...) {
        ring_->waitIdleNoThrow();
        destroy();
    }
}

/**
 **/
VkDescriptorSetLayout TextureArray::layout() const noexcept {
    return layout_;
}

/**
 **/
VkDescriptorSet TextureArray::set() const noexcept {
    return set_;
}

/**
 **/
uint32_t TextureArray::capacity() const noexcept {
    return capacity_;
}

/**
 **/
uint32_t TextureArray::add(const VkDescriptorImageInfo& image) {
    if (free_->empty()) {
        throw std::runtime_error("Every slot of the texture array is taken");
    }
    const uint32_t slot = free_->back();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set_;
    write.dstBinding = 0;
    write.dstArrayElement = slot;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);

    free_->pop_back();
    return slot;
}

/**
 **/
void TextureArray::release(uint32_t slot) {
    ring_->retire([free = free_, slot]() { free->push_back(slot); });
}

/**
 **/
uint32_t TextureArray::taken() const noexcept {
    return capacity_ - static_cast<uint32_t>(free_->size());
}

};  // namespace v3d::render::realtime::vulkan::pipeline
