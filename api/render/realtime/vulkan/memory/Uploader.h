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
 * vertices - is copied there by a command buffer that belongs to nothing else. The
 * caller cannot free its staging buffer until that copy has run. This therefore waits,
 * which keeps a staging allocation's lifetime within the function that made it.
 *
 * It waits on a fence for its own submission, not for the queue to go idle, so frames
 * already in flight go on running. An upload therefore does not mean that a frame in flight
 * has finished, and an object a frame may still read is retired through the frame ring
 * rather than destroyed. Recording copies into an object a frame may be reading is the
 * caller's to prevent; every upload in the api copies into a buffer or image it has just made.
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
    VkFence done_;  /**< signalled by each submission, and reset before the next **/
};

};  // namespace v3d::render::realtime::vulkan::memory
