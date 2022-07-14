/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ChunkBufferPool.h"

void ChunkBufferPool::add(size_t chunkId, const boost::shared_ptr<v3d::gl::VertexBuffer> & buffer) {
    pool_[chunkId] = buffer;
}

boost::shared_ptr<v3d::gl::VertexBuffer> ChunkBufferPool::get(size_t chunkId) {
    return pool_[chunkId];
}

void ChunkBufferPool::render() const {
    boost::unordered_map<size_t, boost::shared_ptr<v3d::gl::VertexBuffer> >::const_iterator it = pool_.begin();
    for (; it != pool_.end(); it++) {
        (*it).second->render();
    }
}
