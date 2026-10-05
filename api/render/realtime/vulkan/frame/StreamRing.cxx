/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "StreamRing.h"

#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
StreamRing::StreamRing(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring,
    VkDeviceSize vertexBytes, VkDeviceSize indexBytes) :
    device_(device),
    ring_(ring),
    vertexBytes_(vertexBytes),
    indexBytes_(indexBytes) {
    slots_.resize(ring_->framesInFlight() > 0 ? ring_->framesInFlight() : 1);
}

/**
 **/
StreamRing::Geometry StreamRing::claim(VkDeviceSize vertexBytes, VkDeviceSize indexBytes) {
    // the device may still be reading what this slot held framesInFlight frames ago
    ring_->waitFrame();
    // a frame that was skipped never began, and its claims were never drawn, so the next
    // frame reuses them rather than claiming after them
    if (ring_->turns() != counted_) {
        counted_ = ring_->turns();
        cursor_ = 0;
    }

    std::vector<Geometry>& slot = slots_[ring_->frame()];
    if (cursor_ >= slot.size()) {
        Geometry geometry;
        geometry.vertices = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBytes_);
        if (indexBytes_ > 0) {
            geometry.indices = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBytes_);
        }
        slot.push_back(geometry);
    }
    Geometry& claimed = slot[cursor_++];
    fit(&claimed.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBytes);
    if (claimed.indices) {
        fit(&claimed.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBytes);
    }
    return claimed;
}

/**
 **/
std::size_t StreamRing::held() const noexcept {
    std::size_t count = 0;
    for (const std::vector<Geometry>& slot : slots_) {
        count += slot.size();
    }
    return count;
}

/**
 **/
void StreamRing::fit(boost::shared_ptr<memory::Buffer>* held, VkBufferUsageFlags usage, VkDeviceSize bytes) {
    const VkDeviceSize size = (*held)->size();
    if (bytes <= size) {
        return;
    }
    VkDeviceSize target = size > 0 ? size : 1;
    while (target < bytes) {
        target *= 2;
    }
    // the old buffer is held by the retirement until no frame in flight can still read it
    const boost::shared_ptr<memory::Buffer> old = *held;
    ring_->retire([old]() {});
    *held = boost::make_shared<memory::Buffer>(device_, usage, target);
}

};  // namespace v3d::render::realtime::vulkan::frame
