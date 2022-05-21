/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include <vertical3d/gl/VertexBuffer.h>

#include <boost/shared_ptr.hpp>

class MeshCache;

/**
 * A class for building GPU buffer data from a mesh cache
 */
class VertexBufferBuilder
{
	public:
		boost::shared_ptr<v3D::VertexBuffer> build(boost::shared_ptr<MeshCache> & mesh);
};