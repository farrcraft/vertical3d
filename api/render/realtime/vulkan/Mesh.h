/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

#include "DeviceBuffer.h"
#include "Device.h"
#include "Uploader.h"

#include "../DrawItem.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

/**
 * Static geometry in device local memory - vertices, and indices where a draw is indexed.
 *
 * A mesh is the one thing a draw item names that Resources does not own, per ADR-0010:
 * pipelines, materials and textures are built at load time and live until the context
 * does, while meshes are created and thrown away while the app runs. So a mesh is held by
 * whatever built it - a chunk, a model - and a draw item referring to one is only valid
 * while that owner is alive.
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
     * @param vertexBytes its size
     * @param vertexCount how many vertices that is - the stride is the pipeline's, not
     *        something a mesh knows, so the count cannot be worked out from the size
     * @param indices 32 bit indices into it
     * @param indexCount how many
     * @throw std::runtime_error if either buffer cannot be created or filled
     **/
    Mesh(const boost::shared_ptr<Device>& device, const boost::shared_ptr<Uploader>& uploader,
        const void* vertices, VkDeviceSize vertexBytes, uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount);

    /**
     * A mesh drawn straight out of its vertex buffer.
     **/
    Mesh(const boost::shared_ptr<Device>& device, const boost::shared_ptr<Uploader>& uploader,
        const void* vertices, VkDeviceSize vertexBytes, uint32_t vertexCount);

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
     * Fill in a draw item's geometry - the buffers, the index type and the counts.
     * What the item draws with, and where it sorts, are left to the caller.
     **/
    void describe(DrawItem* item) const;

 private:
    boost::shared_ptr<DeviceBuffer> vertices_;
    boost::shared_ptr<DeviceBuffer> indices_;
    uint32_t vertexCount_;
    uint32_t indexCount_;
};

};  // namespace v3d::render::realtime::vulkan
