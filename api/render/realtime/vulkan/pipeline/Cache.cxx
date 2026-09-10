/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Cache.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 **/
Cache::Cache(const boost::shared_ptr<device::Device>& device) :
    device_(device),
    cache_(VK_NULL_HANDLE) {
    VkPipelineCacheCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    VkResult result = vkCreatePipelineCache(device_->handle(), &createInfo, nullptr, &cache_);
    if (result != VK_SUCCESS) {
        cache_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create the vulkan pipeline cache - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
Cache::~Cache() {
    if (cache_ != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(device_->handle(), cache_, nullptr);
        cache_ = VK_NULL_HANDLE;
    }
}

/**
 **/
VkPipelineCache Cache::handle() const noexcept {
    return cache_;
}

};  // namespace v3d::render::realtime::vulkan::pipeline
