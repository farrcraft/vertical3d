/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "StreamRing.h"

#include <algorithm>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
StreamRing::StreamRing(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring, const Sizes& sizes) :
    device_(device),
    ring_(ring),
    vertexBytes_(sizes.vertexBytes),
    indexBytes_(sizes.indexBytes) {
    slots_.resize(ring_->framesInFlight() > 0 ? ring_->framesInFlight() : 1);
}

/**
 **/
StreamRing::Geometry StreamRing::claim(const Sizes& sizes) {
    const VkDeviceSize vertexBytes = sizes.vertexBytes;
    const VkDeviceSize indexBytes = sizes.indexBytes;
    // the device may still be reading what this slot held framesInFlight frames ago
    ring_->waitFrame();
    // claims restart whenever the ring turns or moves to another slot. A frame that was
    // skipped, or begun and abandoned, was never drawn, so the next attempt reuses its claims
    // rather than claiming after them
    if (ring_->turns() != counted_ || ring_->frame() != countedFrame_) {
        counted_ = ring_->turns();
        countedFrame_ = ring_->frame();
        cursor_ = 0;
    }

    std::vector<Geometry>& slot = slots_[ring_->frame()];
    // the cursor never passes the end of the slot it indexes, so the claim below either takes
    // a held pair or appends exactly one
    cursor_ = std::min(cursor_, slot.size());
    if (cursor_ == slot.size()) {
        Geometry geometry;
        geometry.vertices = boost::make_shared<memory::Buffer>(device_, memory::BufferSpec{.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, .bytes = vertexBytes_});
        if (indexBytes_ > 0) {
            geometry.indices = boost::make_shared<memory::Buffer>(device_, memory::BufferSpec{.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT, .bytes = indexBytes_});
        }
        slot.push_back(geometry);
    }
    Geometry& claimed = slot[cursor_++];
    fit(&claimed.vertices, {.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, .bytes = vertexBytes});
    if (claimed.indices) {
        fit(&claimed.indices, {.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT, .bytes = indexBytes});
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
void StreamRing::fit(boost::shared_ptr<memory::Buffer>* held, const memory::BufferSpec& spec) {
    const VkDeviceSize size = (*held)->size();
    if (spec.bytes <= size) {
        return;
    }
    VkDeviceSize target = size > 0 ? size : 1;
    while (target < spec.bytes) {
        target *= 2;
    }
    // the old buffer is held by the retirement until no frame in flight can still read it
    const boost::shared_ptr<memory::Buffer> old = *held;
    ring_->retire([old]() {});
    *held = boost::make_shared<memory::Buffer>(device_, memory::BufferSpec{.usage = spec.usage, .bytes = target});
}

};  // namespace v3d::render::realtime::vulkan::frame
