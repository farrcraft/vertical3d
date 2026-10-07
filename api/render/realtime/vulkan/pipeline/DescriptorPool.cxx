/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DescriptorPool.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 **/
DescriptorPool::DescriptorPool(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring,
    const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t setsPerPool, const std::string& name) :
    device_(device),
    ring_(ring),
    name_(name),
    setsPerPool_(setsPerPool > 0 ? setsPerPool : 1),
    layout_(VK_NULL_HANDLE),
    remaining_(0),
    spare_(boost::make_shared<std::vector<VkDescriptorSet>>()) {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    const VkResult result = vkCreateDescriptorSetLayout(device_->handle(), &info, nullptr, &layout_);
    if (result != VK_SUCCESS) {
        layout_ = VK_NULL_HANDLE;
        throw device::failure(result, "Unable to create the " + name_ + " descriptor set layout");
    }

    // a pool holds enough of each type for every set it is created with
    for (const VkDescriptorSetLayoutBinding& binding : bindings) {
        bool counted = false;
        for (VkDescriptorPoolSize& size : sizes_) {
            if (size.type == binding.descriptorType) {
                size.descriptorCount += binding.descriptorCount * setsPerPool_;
                counted = true;
            }
        }
        if (!counted) {
            sizes_.push_back(VkDescriptorPoolSize{binding.descriptorType, binding.descriptorCount * setsPerPool_});
        }
    }
}

/**
 **/
DescriptorPool::~DescriptorPool() {
    const boost::shared_ptr<device::Device> device = device_;
    VkDescriptorSetLayout layout = layout_;
    const auto destroy = [device, layout](const std::vector<VkDescriptorPool>& pools) {
        for (VkDescriptorPool pool : pools) {
            vkDestroyDescriptorPool(device->handle(), pool, nullptr);
        }
        if (layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device->handle(), layout, nullptr);
        }
    };
    // a set from here may be bound by a frame already queued or in flight, so the pools go once
    // those frames have finished. If the ring cannot hold them, they go once the device is idle
    try {
        ring_->retire([destroy, pools = pools_]() { destroy(pools); });
    } catch (...) {
        ring_->waitIdleNoThrow();
        destroy(pools_);
    }
}

/**
 **/
VkDescriptorSetLayout DescriptorPool::layout() const noexcept {
    return layout_;
}

/**
 **/
void DescriptorPool::addPool() {
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets = setsPerPool_;
    info.poolSizeCount = static_cast<uint32_t>(sizes_.size());
    info.pPoolSizes = sizes_.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    const VkResult result = vkCreateDescriptorPool(device_->handle(), &info, nullptr, &pool);
    device::check(result, "Unable to create a " + name_ + " descriptor pool");

    pools_.push_back(pool);
    remaining_ = setsPerPool_;
}

/**
 **/
VkDescriptorSet DescriptorPool::allocate() {
    if (!spare_->empty()) {
        VkDescriptorSet set = spare_->back();
        spare_->pop_back();
        return set;
    }

    if (pools_.empty() || remaining_ == 0) {
        addPool();
    }

    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pools_.back();
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout_;

    VkDescriptorSet set = VK_NULL_HANDLE;
    const VkResult result = vkAllocateDescriptorSets(device_->handle(), &info, &set);
    device::check(result, "Unable to allocate a " + name_ + " descriptor set");
    remaining_--;
    return set;
}

/**
 **/
void DescriptorPool::release(VkDescriptorSet set) {
    if (set == VK_NULL_HANDLE) {
        return;
    }
    ring_->retire([spare = spare_, set]() { spare->push_back(set); });
}

/**
 **/
std::size_t DescriptorPool::pools() const noexcept {
    return pools_.size();
}

};  // namespace v3d::render::realtime::vulkan::pipeline
