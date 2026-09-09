/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/CommandPool.h>

#include <vulkan/vulkan.h>

#include <functional>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * Runs one-shot transfer commands on the graphics queue and waits for them.
 *
 * Everything that has to reach device local memory - a texture's pixels, a mesh's
 * vertices - is copied there by a command buffer that belongs to nothing else, and the
 * caller cannot free its staging buffer until that copy has run. So this waits, which
 * keeps a staging allocation's lifetime to the function that made it.
 *
 * Waiting is only reasonable because uploads happen at load time, off the frame loop.
 * Anything uploading while frames are being drawn wants a transfer queue and a fence
 * rather than this.
 *
 * The one command buffer is reset and reused, so a thousand chunk meshes cost one
 * allocation rather than a thousand the pool never hands back.
 **/
class Uploader final {
 public:
    /**
     * @param device the device whose graphics queue the commands are submitted to
     **/
    explicit Uploader(const boost::shared_ptr<device::Device>& device);

    /**
     **/
    ~Uploader();

    Uploader(const Uploader&) = delete;
    Uploader& operator=(const Uploader&) = delete;

    /**
     * Record, submit and wait for one command buffer.
     * @param record fills the begun buffer with the transfer to run
     * @throw std::runtime_error if the recording or the submission fails
     **/
    void oneShot(const std::function<void(VkCommandBuffer)>& record) const;

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<frame::CommandPool> pool_;
    VkCommandBuffer commands_;
};

};  // namespace v3d::render::realtime::vulkan::memory
