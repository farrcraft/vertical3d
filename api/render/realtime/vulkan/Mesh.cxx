/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mesh.h"

#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

/**
 **/
Mesh::Mesh(const boost::shared_ptr<Device>& device, const boost::shared_ptr<Uploader>& uploader,
    const void* vertices, VkDeviceSize vertexBytes, uint32_t vertexCount) :
    vertexCount_(vertexCount),
    indexCount_(0) {
    if (vertices == nullptr || vertexBytes == 0) {
        throw std::runtime_error("A mesh needs vertices to be created from");
    }
    vertices_ = boost::make_shared<DeviceBuffer>(device, uploader, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, vertexBytes);
}

/**
 **/
Mesh::Mesh(const boost::shared_ptr<Device>& device, const boost::shared_ptr<Uploader>& uploader,
    const void* vertices, VkDeviceSize vertexBytes, uint32_t vertexCount,
    const uint32_t* indices, uint32_t indexCount) :
    Mesh(device, uploader, vertices, vertexBytes, vertexCount) {
    if (indices == nullptr || indexCount == 0) {
        throw std::runtime_error("An indexed mesh needs indices to be created from");
    }
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(indexCount) * sizeof(uint32_t);
    indices_ = boost::make_shared<DeviceBuffer>(device, uploader, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, bytes);
    indexCount_ = indexCount;
}

/**
 **/
Mesh::~Mesh() {
}

/**
 **/
VkBuffer Mesh::vertexBuffer() const noexcept {
    return vertices_ ? vertices_->handle() : VK_NULL_HANDLE;
}

/**
 **/
VkBuffer Mesh::indexBuffer() const noexcept {
    return indices_ ? indices_->handle() : VK_NULL_HANDLE;
}

/**
 **/
uint32_t Mesh::vertexCount() const noexcept {
    return vertexCount_;
}

/**
 **/
uint32_t Mesh::indexCount() const noexcept {
    return indexCount_;
}

/**
 **/
void Mesh::describe(DrawItem* item) const {
    if (item == nullptr) {
        return;
    }
    item->vertexBuffer = vertexBuffer();
    item->vertexBufferOffset = 0;
    item->indexBuffer = indexBuffer();
    item->indexBufferOffset = 0;
    item->indexType = VK_INDEX_TYPE_UINT32;
    item->indices = indexCount_;
    // an indexed draw takes its count from the indices, so the vertex count is only the
    // draw's when there are none
    item->vertices = indexCount_ > 0 ? 0 : vertexCount_;
    item->instances = 1;
}

};  // namespace v3d::render::realtime::vulkan
