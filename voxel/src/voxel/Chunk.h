/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include "Voxel.h"
#include "TerrainMap.h"

#include <glm/glm.hpp>
#include <vector>
#include <boost/unordered_map.hpp>

class Chunk
{
	public:
		Chunk(TerrainMap & terrain, glm::ivec3 chunkPosition, unsigned int ceiling);

		void update();
		void render();

		void invalidate();
		void dirty(bool state);

		/**
		 * The number of voxels in single dimension of the chunk volume
		 */
		unsigned int size() const;

		boost::unordered_map<unsigned int, boost::shared_ptr<Voxel> > & blocks();

		bool empty() const;
		bool dirty() const;

		/**
		 * Check to see if a neighboring block exists with an opposing face that makes the checking block's face hidden
		 *
		 * @param face the block's face to check
		 * @param position the position of the block
		 */
		bool hidden(Voxel::BlockFace face, const glm::ivec3 & position);

		glm::ivec3 position() const;

		bool active(glm::ivec3 blockPosition) const;

	private:
		boost::unordered_map<unsigned int, boost::shared_ptr<Voxel> > blocks_;
		glm::ivec3 position_;
		bool dirty_;
		bool empty_;
		unsigned int size_;
};
