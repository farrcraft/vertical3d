/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class Chunk;
class ChunkBufferPool;
class MeshCache;

class MeshBuilder
{
	public:
		MeshBuilder(boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > > & chunks);

		void build(const boost::shared_ptr<ChunkBufferPool> & pool, size_t limit);

	protected:
		/**
		 * Genereate mesh geometry for a chunk
		 */
		void generateChunk(const boost::shared_ptr<ChunkBufferPool> & pool, boost::shared_ptr<Chunk> & chunk);

	private:
		boost::shared_ptr<MeshCache> cache_;
		boost::unordered_map<unsigned int, boost::shared_ptr<Chunk > > chunks_;
};
