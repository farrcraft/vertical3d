/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>

#include <boost/shared_ptr.hpp>

class MeshCache;

/**
 * Turns a built mesh cache into device local geometry.
 *
 * A chunk is meshed once and drawn for the life of the process, so its geometry goes in a
 * vulkan::memory::DeviceBuffer: one staging copy at build time, and the fastest memory for
 * every frame after it.
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
