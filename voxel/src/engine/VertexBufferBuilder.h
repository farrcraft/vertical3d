/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../v3dlibs/gl/VertexBuffer.h"

#include <boost/shared_ptr.hpp>

class MeshCache;

/**
 * A class for building GPU buffer data from a mesh cache
 */
class VertexBufferBuilder {
 public:
    boost::shared_ptr<v3d::gl::VertexBuffer> build(const boost::shared_ptr<MeshCache> & mesh);
};
