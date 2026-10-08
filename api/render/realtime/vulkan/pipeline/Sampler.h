/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * How a shader reads an image: filtering, what lies past the edge, and how far down the
 * mip chain it may go.
 *
 * A sampler names no image, so one is shared by everything read the same way. It is held
 * by shared pointer for that reason, and so that a frame in flight can keep it alive.
 **/
class Sampler final {
 public:
    /**
     * What to make. The defaults are what a texture and a render target are read through. They
     * are linear, because both are drawn at sizes other than their own. They are clamped to
     * the edge, because a region's neighbour in an atlas is another sprite and wrapping would
     * bleed it in.
     **/
    struct Spec final {
        Spec() noexcept;

        VkFilter filter;
        VkSamplerMipmapMode mipmap;
        VkSamplerAddressMode address;  /**< in all three directions **/
        VkBorderColor border;          /**< read only when address clamps to the border **/
        float maxLod;

        /**
         * Two specs are equal when every field is, so they would make the same sampler.
         **/
        bool operator==(const Spec& other) const noexcept = default;
    };

    /**
     * @param device the device to create the sampler on
     * @param spec how to read
     * @throw std::runtime_error if creation fails
     **/
    Sampler(const boost::shared_ptr<device::Device>& device, const Spec& spec);

    /**
     **/
    ~Sampler();

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    /**
     * @return the sampler, for a descriptor set
     **/
    VkSampler handle() const noexcept;

 private:
    boost::shared_ptr<device::Device> device_;
    VkSampler sampler_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline
