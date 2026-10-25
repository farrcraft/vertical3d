/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/CommandPool.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * Runs transfer commands on the graphics queue without waiting for them.
 *
 * Everything that has to reach device local memory, such as a texture's pixels or a mesh's
 * vertices, is copied there by a command buffer that belongs to nothing else. submit() records
 * it, submits it and returns. What the copy reads, its staging buffer above all, is handed over
 * with it and retired through the Ring with the command buffer. Both are freed once the frame
 * being recorded has finished, and that frame's fence covers every submission made before it on
 * the queue.
 *
 * The copy is ordered before every later submission on the queue, so a frame submitted after it
 * sees its result if the copy ends with a barrier that makes its writes visible. Every upload in
 * the api ends with one. A finished upload does not mean that a frame in flight has finished, so
 * an object a frame may still read is retired through the Ring, not destroyed. The caller must
 * not record a copy into an object a frame may be reading. Every upload in the api copies into a
 * buffer or image it has just made.
 *
 * Nothing an upload holds is freed until frames are begun, so a caller that uploads without
 * drawing holds every staging buffer until the Ring is destroyed.
 *
 * A command buffer is reused once its frame has finished, so a steady run of uploads allocates
 * only as many as are in flight at once. An Uploader is used from the thread that begins
 * frames, because the Ring frees what it retired on that thread. It may be destroyed while its
 * uploads are in flight: the pool they came from is kept until the last is retired.
 **/
class Uploader final {
 public:
    /**
     * @param device the device whose graphics queue the commands are submitted to
     * @param ring the frames whose finishing frees what each upload held
     **/
    Uploader(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring);

    Uploader(const Uploader&) = delete;
    Uploader& operator=(const Uploader&) = delete;

    /**
     * Record and submit one command buffer, and return without waiting for it.
     *
     * @param record fills the begun buffer with the transfer to run
     * @param keep what the transfer reads, held until the frame being recorded has finished.
     *        Null when it reads nothing the caller would otherwise free
     * @throw std::runtime_error if the recording or the submission fails. Nothing is submitted
     *        then, and keep is released
     **/
    void submit(const std::function<void(VkCommandBuffer)>& record, boost::shared_ptr<void> keep = {});

 private:
    /**
     * The pool the command buffers come from and the ones ready to use again. It is shared with
     * every retirement in flight, so it outlives the Uploader until the last has run.
     **/
    class Spare final {
     public:
        boost::shared_ptr<frame::CommandPool> pool;
        std::vector<VkCommandBuffer> ready;
    };

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<frame::Ring> ring_;
    boost::shared_ptr<Spare> spare_;
};

};  // namespace v3d::render::realtime::vulkan::memory
