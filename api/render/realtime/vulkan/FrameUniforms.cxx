/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameUniforms.h"

#include <sstream>
#include <stdexcept>
#include <vector>

#include "Result.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

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
FrameUniforms::FrameUniforms(const boost::shared_ptr<Device>& device, uint32_t framesInFlight) :
    device_(device),
    layout_(VK_NULL_HANDLE),
    remaining_(0),
    frame_(0),
    cursor_(0) {
    createLayout();
    slots_.resize(framesInFlight > 0 ? framesInFlight : 1);
}

/**
 **/
FrameUniforms::~FrameUniforms() {
    // the buffers go with the slots; the sets go with the pools
    slots_.clear();

    for (VkDescriptorPool pool : pools_) {
        vkDestroyDescriptorPool(device_->handle(), pool, nullptr);
    }
    pools_.clear();

    if (layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_->handle(), layout_, nullptr);
        layout_ = VK_NULL_HANDLE;
    }
}

/**
 **/
void FrameUniforms::createLayout() {
    VkDescriptorSetLayoutBinding camera{};
    camera.binding = 0;
    camera.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camera.descriptorCount = 1;
    // a fragment shader wants the camera as often as a vertex shader does - for a view
    // direction, or for reconstructing a position - so both stages see it
    camera.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &camera;

    VkResult result = vkCreateDescriptorSetLayout(device_->handle(), &info, nullptr, &layout_);
    if (result != VK_SUCCESS) {
        layout_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create the per frame descriptor set layout - " << resultString(result);
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
void FrameUniforms::addPool() {
    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    size.descriptorCount = poolSize;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets = poolSize;
    info.poolSizeCount = 1;
    info.pPoolSizes = &size;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorPool(device_->handle(), &info, nullptr, &pool);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create the per frame descriptor pool - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    pools_.push_back(pool);
    remaining_ = poolSize;
}

/**
 **/
FrameUniforms::Slot FrameUniforms::addSlot() {
    if (pools_.empty() || remaining_ == 0) {
        addPool();
    }

    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pools_.back();
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout_;

    Slot slot;
    VkResult result = vkAllocateDescriptorSets(device_->handle(), &info, &slot.set);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate a per frame descriptor set - " << resultString(result);
        throw std::runtime_error(msg.str());
    }
    remaining_--;

    slot.buffer = boost::make_shared<Buffer>(device_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Camera));

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
    return layout_;
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

};  // namespace v3d::render::realtime::vulkan
