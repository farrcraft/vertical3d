/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sampler.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 **/
Sampler::Spec::Spec() noexcept :
filter(VK_FILTER_LINEAR),
mipmap(VK_SAMPLER_MIPMAP_MODE_LINEAR),
address(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE),
border(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK),
maxLod(VK_LOD_CLAMP_NONE) {
}

/**
 **/
Sampler::Sampler(const boost::shared_ptr<device::Device>& device, const Spec& spec) :
    device_(device),
    sampler_(VK_NULL_HANDLE) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = spec.filter;
    info.minFilter = spec.filter;
    info.mipmapMode = spec.mipmap;
    info.addressModeU = spec.address;
    info.addressModeV = spec.address;
    info.addressModeW = spec.address;
    info.borderColor = spec.border;
    info.maxLod = spec.maxLod;

    const VkResult result = vkCreateSampler(device_->handle(), &info, nullptr, &sampler_);
    if (result != VK_SUCCESS) {
        sampler_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create a vulkan sampler - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
Sampler::~Sampler() {
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_->handle(), sampler_, nullptr);
    }
}

/**
 **/
VkSampler Sampler::handle() const noexcept {
    return sampler_;
}

};  // namespace v3d::render::realtime::vulkan::pipeline
