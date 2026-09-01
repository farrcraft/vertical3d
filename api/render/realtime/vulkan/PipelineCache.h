/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

    /**
     * The driver's cache of compiled pipeline state.
     *
     * Every pipeline the engine builds is created against this, so that pipelines sharing
     * shader stages or state pay for the compilation once. It exists from the first frame
     * rather than being added when it starts to matter, because a pipeline created outside
     * the cache is not retroactively put into it.
     *
     * Nothing writes the cache to disk yet. When that lands it is a serialize on shutdown
     * and a load on startup, and the format is opaque and driver specific - a cache from
     * another device or driver version is rejected, which is the caller's problem to handle.
     **/
    class PipelineCache final {
     public:
        /**
         * @param device the device pipelines are compiled for
         **/
        explicit PipelineCache(const boost::shared_ptr<Device>& device);

        /**
         **/
        ~PipelineCache();

        PipelineCache(const PipelineCache&) = delete;
        PipelineCache& operator=(const PipelineCache&) = delete;

        /**
         * @return the underlying cache handle, to be passed to every pipeline creation
         **/
        VkPipelineCache handle() const noexcept;

     private:
        boost::shared_ptr<Device> device_;
        VkPipelineCache cache_;
    };

};  // namespace v3d::render::realtime::vulkan
