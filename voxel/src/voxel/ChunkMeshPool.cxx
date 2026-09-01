/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ChunkMeshPool.h"

ChunkMeshPool::Entry::Entry() :
    origin(0.0f, 0.0f, 0.0f) {
}

void ChunkMeshPool::add(size_t chunkId, const boost::shared_ptr<v3d::render::realtime::vulkan::Mesh>& mesh, const glm::vec3& origin) {
    Entry entry;
    entry.mesh = mesh;
    entry.origin = origin;
    pool_[chunkId] = entry;
}

const ChunkMeshPool::EntryMap& ChunkMeshPool::entries() const {
    return pool_;
}
