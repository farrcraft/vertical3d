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
 * The driver's cache of compiled pipeline state.
 *
 * Every pipeline the engine builds is created against this, so that pipelines sharing
 * shader stages or state pay for the compilation once. A pipeline created outside the
 * cache is not retroactively put into it, which is why the cache exists from the first
 * frame rather than being added when it starts to matter.
 *
 * Nothing writes the cache to disk, so it lives only as long as the process.
 **/
class Cache final {
 public:
    /**
     * @param device the device pipelines are compiled for
     **/
    explicit Cache(const boost::shared_ptr<device::Device>& device);

    /**
     **/
    ~Cache();

    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    /**
     * @return the underlying cache handle, to be passed to every pipeline creation
     **/
    VkPipelineCache handle() const noexcept;

 private:
    boost::shared_ptr<device::Device> device_;
    VkPipelineCache cache_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline
