/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ChunkMeshPool.h"

#include <utility>

ChunkMeshPool::Entry::Entry() :
    origin(0.0F, 0.0F, 0.0F),
    size(0.0F) {
}

ChunkMeshPool::ChunkMeshPool(Retire retire) :
    retire_(std::move(retire)) {
}

void ChunkMeshPool::add(size_t chunkId, const boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh>& mesh, const glm::vec3& origin,
    float size) {
    Entry entry;
    entry.mesh = mesh;
    entry.origin = origin;
    entry.size = size;

    const EntryMap::iterator found = pool_.find(chunkId);
    if (found == pool_.end()) {
        pool_.emplace(chunkId, entry);
        return;
    }
    boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh> replaced = found->second.mesh;
    found->second = entry;
    retire_([replaced]() mutable { replaced.reset(); });
}

const ChunkMeshPool::EntryMap& ChunkMeshPool::entries() const {
    return pool_;
}
