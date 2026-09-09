/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class MeshCache;

/**
 * One vertex of a chunk mesh, in the layout shaders/voxel.vert declares.
 *
 * The position is chunk local, so a chunk's vertices never change when it moves and the
 * origin the pipeline adds back is a push constant rather than a rebuild.
 **/
struct ChunkVertex {
    glm::vec3 position;
    glm::vec2 info;  /**< the face bit, and the block type it was cut from **/
};

/**
 * Turns a built mesh cache into device local geometry.
 *
 * A chunk is meshed once and drawn for the life of the process, which is the trade
 * vulkan::memory::DeviceBuffer is for - one staging copy at build time against the fastest memory
 * for every frame after it.
 */
class ChunkMeshBuilder {
 public:
    /**
     * @param device the device the geometry is allocated on
     * @param uploader runs the staging copies
     **/
    ChunkMeshBuilder(const boost::shared_ptr<v3d::render::realtime::vulkan::device::Device>& device,
        const boost::shared_ptr<v3d::render::realtime::vulkan::memory::Uploader>& uploader);

    /**
     * @param mesh the faces and triangles to upload, which are not kept
     * @return the mesh, or null when the cache holds no geometry
     * @throw std::runtime_error if the buffers cannot be created or filled
     **/
    boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh> build(const boost::shared_ptr<MeshCache>& mesh) const;

 private:
    boost::shared_ptr<v3d::render::realtime::vulkan::device::Device> device_;
    boost::shared_ptr<v3d::render::realtime::vulkan::memory::Uploader> uploader_;
};
