/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mesh.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/DeviceBuffer.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

/**
 **/
Mesh::Mesh(const boost::shared_ptr<vulkan::device::Device>& device, const boost::shared_ptr<vulkan::memory::Uploader>& uploader,
    std::span<const std::byte> vertices, uint32_t vertexCount) :
    vertexCount_(vertexCount),
    indexCount_(0) {
    if (vertices.empty()) {
        throw std::runtime_error("A mesh needs vertices to be created from");
    }
    vertices_ = boost::make_shared<vulkan::memory::DeviceBuffer>(device, uploader, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data(),
        static_cast<VkDeviceSize>(vertices.size()));
}

/**
 **/
Mesh::Mesh(const boost::shared_ptr<vulkan::device::Device>& device, const boost::shared_ptr<vulkan::memory::Uploader>& uploader,
    std::span<const std::byte> vertices, uint32_t vertexCount, std::span<const uint32_t> indices) :
    Mesh(device, uploader, vertices, vertexCount) {
    if (indices.empty()) {
        throw std::runtime_error("An indexed mesh needs indices to be created from");
    }
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(indices.size_bytes());
    indices_ = boost::make_shared<vulkan::memory::DeviceBuffer>(device, uploader, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.data(), bytes);
    indexCount_ = static_cast<uint32_t>(indices.size());
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

};  // namespace v3d::render::realtime
