/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../../api/gl/VertexBuffer.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class ChunkBufferPool {
 public:
    void add(size_t chunkId, const boost::shared_ptr<v3d::gl::VertexBuffer> & buffer);
    boost::shared_ptr<v3d::gl::VertexBuffer> get(size_t chunkId);
    void render() const;

 private:
      boost::unordered_map<size_t, boost::shared_ptr<v3d::gl::VertexBuffer> > pool_;
};
