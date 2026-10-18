/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#pragma once

#include <vector>

#include "Voxel.h"
#include "TerrainMap.h"

#include <glm/glm.hpp>
#include <boost/unordered_map.hpp>

class Chunk {
 public:
    Chunk(TerrainMap * terrain, glm::ivec3 chunkPosition, unsigned int ceiling);

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
        * Whether a block of this chunk sits against a block's face, so the face cannot be seen.
        * A face on the chunk's edge is never hidden here: the neighbour is in the next chunk.
        *
        * @param face the block's face to check
        * @param position the block's position within this chunk
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
