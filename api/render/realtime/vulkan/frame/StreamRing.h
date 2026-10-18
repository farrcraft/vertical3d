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
 * Buffers that a renderer streams a frame's geometry through, with one set per frame in flight.
 *
 * Each claim() takes the next vertex and index buffer pair from the set of the frame being
 * recorded. A frame holds as many pairs as it has claimed. The Ring identifies the frame. The
 * first claim after the Ring begins, begins again, skips or advances starts from the front of
 * the set, so nothing has to signal the end of a frame. A renderer that an app creates reuses
 * its buffers the same way as one the engine holds.
 *
 * When content outgrows a buffer, the buffer is replaced by one doubled in size until it fits.
 * The old buffer is retired through the Ring, so it is not destroyed while the device may read
 * it.
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
     * Sizes in bytes of a vertex buffer and an index buffer. Given to the constructor, they are
     * the starting sizes, and an index size of zero means no index buffers are made.
     **/
    struct Sizes {
        VkDeviceSize vertexBytes = 0;
        VkDeviceSize indexBytes = 0;
    };

    StreamRing(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring, const Sizes& sizes);

    StreamRing(const StreamRing&) = delete;
    StreamRing& operator=(const StreamRing&) = delete;

    /**
     * Wait until the frame being recorded may write its buffers again, then take the next pair
     * of them, at least this large.
     **/
    Geometry claim(const Sizes& sizes);

    /**
     * @return how many buffer pairs are held across every frame in flight. Each slot holds as
     *         many as its busiest frame claimed, and the count does not grow with the number
     *         of frames run.
     **/
    std::size_t held() const noexcept;

 private:
    /**
     * Make a held buffer at least this large, replacing and retiring it when it is not.
     **/
    void fit(boost::shared_ptr<memory::Buffer>* held, const memory::BufferSpec& spec);

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Ring> ring_;
    VkDeviceSize vertexBytes_;
    VkDeviceSize indexBytes_;
    std::vector<std::vector<Geometry>> slots_;  /**< a set of geometry per frame in flight, grown as a frame claims more **/
    std::size_t cursor_ = 0;                    /**< how far into the current frame's set the claims have got **/
    uint64_t counted_ = std::numeric_limits<uint64_t>::max();  /**< the Ring's turns() when cursor_ was last reset **/
    uint32_t countedFrame_ = std::numeric_limits<uint32_t>::max();  /**< the Ring's frame() when cursor_ was last reset **/
};

};  // namespace v3d::render::realtime::vulkan::frame
