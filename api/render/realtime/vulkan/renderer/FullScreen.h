/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::renderer {

/**
 * A pass over the whole of a target, one fragment per pixel: a triangle covering it, and a
 * fragment stage of the caller's reading images bound at set 1. A colour grade is one, and so
 * is any later post pass - a blur, a tone map, a vignette.
 *
 * The images are the source, named as a material, so the item binds them the way any draw
 * binds a texture. Set 0 is declared, as every pipeline in the engine declares it, and need
 * not be read. The pass that draws it must name what it samples in Pass::reads(), so the
 * frame records whatever drew them first.
 **/
class FullScreen final {
 public:
    /**
     * What one full-screen pipeline is.
     **/
    struct Spec final {
        std::string name;
        std::vector<uint32_t> fragment;  /**< SPIR-V; it reads uv at location 0 **/
        uint32_t sources = 1;            /**< combined image samplers at set 1, bindings 0 upwards **/
        VkFormat colour = VK_FORMAT_UNDEFINED;  /**< what the pass draws into **/
        /**
         * The depth format of the pass it is drawn in, which it neither tests nor writes but is
         * still built against, or undefined for a pass with no depth.
         **/
        VkFormat depth = VK_FORMAT_UNDEFINED;
    };

    /**
     * @throw std::runtime_error if the pipeline cannot be built, or names no source
     **/
    FullScreen(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<pipeline::Cache>& cache,
        const boost::shared_ptr<pipeline::Resources>& resources, const boost::shared_ptr<frame::Ring>& ring,
        const boost::shared_ptr<frame::FrameUniforms>& uniforms, const Spec& spec);

    FullScreen(const FullScreen&) = delete;
    FullScreen& operator=(const FullScreen&) = delete;

    /**
     * Bind images as a source, each read through the sampler it was registered with. A depth
     * image is bound in DEPTH_READ_ONLY_OPTIMAL, the layout the recorder leaves a sampled
     * depth image in.
     *
     * A source names its images as they are now. A target that is resized, or a texture that
     * is released, is bound again rather than followed. Release a source before this goes,
     * since the set it names is this one's.
     *
     * @param textures one per binding the spec declared, in binding order
     * @return the material an item names
     * @throw std::runtime_error if the count is wrong or a handle names nothing
     **/
    MaterialHandle source(const std::vector<TextureHandle>& textures);

    /**
     * Let a source go, its set returned through the ring once no frame can be reading it.
     *
     * @return whether there was anything to release
     **/
    bool release(const MaterialHandle& source);

    /**
     * @return the one draw that covers the pass, reading the source given
     **/
    DrawItem item(const MaterialHandle& source) const;

    /**
     * Submit item() into a pass.
     **/
    void submit(const MaterialHandle& source, Pass* pass) const;

    /**
     * @return the pipeline
     **/
    PipelineHandle pipeline() const noexcept;

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<pipeline::DescriptorPool> sources_;
    uint32_t count_;
    PipelineHandle pipeline_;
};

};  // namespace v3d::render::realtime::vulkan::renderer
