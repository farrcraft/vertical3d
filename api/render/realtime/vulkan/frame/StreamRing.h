/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "Ring.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The buffers a renderer streams a frame's geometry through, a set per frame in flight.
 *
 * A submission claims the next pair of the frame being recorded, and a frame takes as many as
 * it submits. The ring identifies the frame. Claims restart from the front of a slot the first
 * time it is claimed from after the ring has begun another frame, so nothing has to signal
 * that a frame ended. A renderer an app built itself reuses its buffers the same way as one
 * the engine holds.
 *
 * A buffer the content outgrows is replaced by one twice the size, and the old one is retired
 * through the ring rather than destroyed while the device may be reading it.
 **/
class StreamRing final {
 public:
    /**
     * One submission's buffers. The index buffer is null for geometry drawn without one.
     **/
    struct Geometry {
        boost::shared_ptr<memory::Buffer> vertices;
        boost::shared_ptr<memory::Buffer> indices;
    };

    /**
     * @param vertexBytes what each vertex buffer starts at
     * @param indexBytes what each index buffer starts at, or zero for geometry with none
     **/
    StreamRing(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring,
        VkDeviceSize vertexBytes, VkDeviceSize indexBytes);

    StreamRing(const StreamRing&) = delete;
    StreamRing& operator=(const StreamRing&) = delete;

    /**
     * Wait until the frame being recorded may write its buffers again, then take the next pair
     * of them, at least this large.
     **/
    Geometry claim(VkDeviceSize vertexBytes, VkDeviceSize indexBytes);

    /**
     * @return how many sets of buffers are held across every frame in flight - as many as the
     *         busiest frame claimed, times the frames, and no more however many frames run
     **/
    std::size_t held() const noexcept;

 private:
    /**
     * Make a held buffer at least this large, replacing and retiring it when it is not.
     **/
    void fit(boost::shared_ptr<memory::Buffer>* held, VkBufferUsageFlags usage, VkDeviceSize bytes);

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Ring> ring_;
    VkDeviceSize vertexBytes_;
    VkDeviceSize indexBytes_;
    std::vector<std::vector<Geometry>> slots_;  /**< a set of geometry per frame in flight, grown as a frame claims more **/
    std::size_t cursor_ = 0;                    /**< how far into the current frame's set the claims have got **/
    uint64_t counted_ = std::numeric_limits<uint64_t>::max();  /**< the ring's turns() the cursor counts in **/
};

};  // namespace v3d::render::realtime::vulkan::frame
