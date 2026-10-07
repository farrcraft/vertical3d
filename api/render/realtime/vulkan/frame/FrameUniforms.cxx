/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameUniforms.h"

#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

namespace {

/**
 * How many sets a descriptor pool is created with. One set per pass per frame in
 * flight, so a pool covers more frames of passes than anything here will ask for.
 **/
const uint32_t poolSize = 32;

};  // namespace

/**
 **/
FrameUniforms::Camera::Camera() noexcept :
view(1.0f),
projection(1.0f),
viewProjection(1.0f),
viewport(0.0f, 0.0f, 0.0f, 0.0f) {
}

/**
 **/
FrameUniforms::Slot::Slot() noexcept :
set(VK_NULL_HANDLE) {
}

/**
 **/
FrameUniforms::FrameUniforms(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring) :
    device_(device),
    frame_(0),
    cursor_(0) {
    VkDescriptorSetLayoutBinding camera{};
    camera.binding = 0;
    camera.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camera.descriptorCount = 1;
    // a fragment shader needs the camera as often as a vertex shader does - for a view
    // direction, or for reconstructing a position - so both stages see it
    camera.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pool_ = boost::make_shared<pipeline::DescriptorPool>(device_, ring, std::vector<VkDescriptorSetLayoutBinding>{camera},
        poolSize, "per frame");

    slots_.resize(ring->framesInFlight() > 0 ? ring->framesInFlight() : 1);
}

/**
 **/
FrameUniforms::Slot FrameUniforms::addSlot() {
    Slot slot;
    slot.set = pool_->allocate();
    slot.buffer = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Camera));

    VkDescriptorBufferInfo buffer{};
    buffer.buffer = slot.buffer->handle();
    buffer.offset = 0;
    buffer.range = sizeof(Camera);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = slot.set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer;

    // the set points at the buffer for good - a frame writes the buffer's contents, not
    // the descriptor
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);

    return slot;
}

/**
 **/
VkDescriptorSetLayout FrameUniforms::layout() const noexcept {
    return pool_->layout();
}

/**
 **/
void FrameUniforms::begin(uint32_t frame) noexcept {
    frame_ = frame;
    cursor_ = 0;
}

/**
 **/
VkDescriptorSet FrameUniforms::write(const Camera& camera) {
    if (frame_ >= slots_.size()) {
        return VK_NULL_HANDLE;
    }

    std::vector<Slot>& ring = slots_[frame_];
    if (cursor_ >= ring.size()) {
        ring.push_back(addSlot());
    }

    const Slot& slot = ring[cursor_];
    cursor_++;

    slot.buffer->write(&camera, sizeof(camera));
    return slot.set;
}

};  // namespace v3d::render::realtime::vulkan::frame
