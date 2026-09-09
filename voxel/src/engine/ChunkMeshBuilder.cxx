/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ChunkMeshBuilder.h"

#include <voxel/src/voxel/MeshCache.h>

#include <cstdint>
#include <vector>

#include <boost/make_shared.hpp>

ChunkMeshBuilder::ChunkMeshBuilder(const boost::shared_ptr<v3d::render::realtime::vulkan::device::Device>& device,
    const boost::shared_ptr<v3d::render::realtime::vulkan::memory::Uploader>& uploader) :
    device_(device),
    uploader_(uploader) {
}

boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh> ChunkMeshBuilder::build(const boost::shared_ptr<MeshCache>& mesh) const {
    const size_t vertexCount = mesh->vertexCount();
    const size_t triCount = mesh->triCount();
    if (vertexCount == 0 || triCount == 0) {
        return boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh>();
    }

    // the face a vertex belongs to is what carries its normal and its material, and a vertex
    // is only ever shared within one face, so the per face pair can be scattered out to the
    // four corners the face added
    std::vector<ChunkVertex> vertices(vertexCount);
    const glm::vec3* positions = mesh->vertices();
    for (size_t i = 0; i < vertexCount; i++) {
        vertices[i].position = positions[i];
        vertices[i].info = glm::vec2(0.0f, 0.0f);
    }

    const glm::ivec3* tris = mesh->tris();
    const glm::ivec4* faces = mesh->faces();
    const size_t faceCount = mesh->faceCount();
    for (size_t i = 0; i < faceCount; i++) {
        const glm::ivec4 face = faces[i];
        const glm::vec2 info(static_cast<float>(face.z), static_cast<float>(face.w));
        // the face's two triangles name its four vertices between them, twice over for the
        // shared edge - assigning the same value to all six costs less than deduplicating
        const glm::ivec3 corners[2] = { tris[face.x], tris[face.y] };
        for (const glm::ivec3& tri : corners) {
            vertices[tri.x].info = info;
            vertices[tri.y].info = info;
            vertices[tri.z].info = info;
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(triCount * 3);
    for (size_t i = 0; i < triCount; i++) {
        indices.push_back(static_cast<uint32_t>(tris[i].x));
        indices.push_back(static_cast<uint32_t>(tris[i].y));
        indices.push_back(static_cast<uint32_t>(tris[i].z));
    }

    return boost::make_shared<v3d::render::realtime::vulkan::memory::Mesh>(device_, uploader_,
        vertices.data(), static_cast<VkDeviceSize>(vertices.size() * sizeof(ChunkVertex)),
        static_cast<uint32_t>(vertices.size()),
        indices.data(), static_cast<uint32_t>(indices.size()));
}
