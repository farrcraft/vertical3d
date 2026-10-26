/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/DeviceBuffer.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <span>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

/**
 * Static geometry in device local memory: vertices, and indices where the draw is indexed.
 *
 * A mesh is the only thing a draw item names that Resources does not own. Whatever built a
 * mesh holds it, such as a chunk, a model or the MeshRegistry. A draw item that refers to one
 * is valid only while that owner is alive.
 *
 * The buffers are filled once at construction. Rebuilding geometry means building a new
 * mesh rather than refilling this one, since a refill has to wait for every frame that
 * might still be drawing the old contents.
 **/
class Mesh final {
 public:
    /**
     * An indexed mesh.
     * @param device the device to allocate on
     * @param uploader runs the staging copies
     * @param vertices the vertex data, in whatever layout the pipeline declares
     * @param vertexCount how many vertices that is. The stride belongs to the pipeline, not
     *        the mesh, so the count cannot be worked out from the size.
     * @param indices 32 bit indices into it
     * @throw std::runtime_error if either buffer cannot be created or filled
     **/
    Mesh(const boost::shared_ptr<vulkan::device::Device>& device, const boost::shared_ptr<vulkan::memory::Uploader>& uploader,
        std::span<const std::byte> vertices, uint32_t vertexCount, std::span<const uint32_t> indices);

    /**
     * A mesh drawn straight out of its vertex buffer.
     **/
    Mesh(const boost::shared_ptr<vulkan::device::Device>& device, const boost::shared_ptr<vulkan::memory::Uploader>& uploader,
        std::span<const std::byte> vertices, uint32_t vertexCount);

    /**
     **/
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    /**
     * @return the vertex buffer handle
     **/
    VkBuffer vertexBuffer() const noexcept;

    /**
     * @return the index buffer handle, or null when the mesh is not indexed
     **/
    VkBuffer indexBuffer() const noexcept;

    /**
     * @return how many vertices the mesh holds
     **/
    uint32_t vertexCount() const noexcept;

    /**
     * @return how many indices it holds, or zero when it is not indexed
     **/
    uint32_t indexCount() const noexcept;

    /**
     * Fill in a draw item's geometry: the buffers, the index type and the counts.
     * What the item draws with, and where it sorts, are left to the caller.
     **/
    void describe(DrawItem* item) const;

 private:
    boost::shared_ptr<vulkan::memory::DeviceBuffer> vertices_;
    boost::shared_ptr<vulkan::memory::DeviceBuffer> indices_;
    uint32_t vertexCount_;
    uint32_t indexCount_;
};

};  // namespace v3d::render::realtime
