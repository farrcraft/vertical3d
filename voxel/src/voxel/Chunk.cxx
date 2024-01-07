/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#include "Chunk.h"
#include "../engine/MortonCode.h"

Chunk::Chunk(TerrainMap & terrain, glm::ivec3 chunkPosition, unsigned int ceiling) :
    dirty_(false),
    size_(16),
    empty_(false),
    position_(chunkPosition) {
    float maxTerrainHeight = 255.0f;
    float voxelHeight = static_cast<float>(ceiling);
    MortonCode encoder;
    unsigned int hash = 0;
    chunkPosition *= size_;
    unsigned int allocated = 0;

    // pick a random block type for this chunk. valid solid block type numbers are [1-16].
    Voxel::BlockType chunkBlockType = static_cast<Voxel::BlockType>(rand() % 16 + 1); //  NOLINT

    for (unsigned int x = 0; x < size_; x++) {
        float posX = static_cast<float>(x + chunkPosition.x);
        for (unsigned int z = 0; z < size_; z++) {
            float posZ = static_cast<float>(z + chunkPosition.z);
            // height is in the range [0.0-255.0]
            float height = terrain.height(x + chunkPosition.x, z + chunkPosition.z);
            unsigned int blockHeight = 0;
            if (height > 0.0f) {
                // scale into the range of the world ceiling
                blockHeight = static_cast<unsigned int>((height / maxTerrainHeight) * voxelHeight);
            }

            for (unsigned y = 0; y < size_; y++) {
                float posY = static_cast<float>(y + chunkPosition.y);
                if ((y + chunkPosition.y) <= blockHeight) {
                    hash = encoder.encode(glm::ivec3(x, y, z));
                    glm::vec3 position(posX, posY, posZ);
                    // Voxel::BlockType type = Voxel::BLOCK_TYPE_DIRT;

                    boost::shared_ptr<Voxel> voxel(new Voxel(chunkBlockType, position));
                    blocks_[hash] = voxel;
                    allocated++;
                }
            }
        }
    }
    if (allocated > 0) {
        dirty_ = true;
    } else {
        empty_ = true;
    }
}

glm::ivec3 Chunk::position() const {
    return position_;
}

bool Chunk::active(glm::ivec3 blockPosition) const {
    MortonCode codec;
    unsigned int hash = codec.encode(blockPosition);
    if (blocks_.find(hash) != blocks_.end()) {
        return true;
    }
    return false;
}


bool Chunk::hidden(Voxel::BlockFace face, const glm::ivec3 & position) {
    unsigned int neighborHash = 0;
    glm::ivec3 neighborPosition = position;
    MortonCode codec;
    switch (face) {
        case Voxel::BLOCK_FACE_BACK:
            neighborPosition.z -= 1;
            break;
        case Voxel::BLOCK_FACE_FRONT:
            neighborPosition.z += 1;
            break;
        case Voxel::BLOCK_FACE_RIGHT:
            neighborPosition.x += 1;
            break;
        case Voxel::BLOCK_FACE_LEFT:
            neighborPosition.x -= 1;
            break;
        case Voxel::BLOCK_FACE_TOP:
            neighborPosition.y += 1;
            break;
        case Voxel::BLOCK_FACE_BOTTOM:
            neighborPosition.y -= 1;
            break;
    }
    neighborHash = codec.encode(neighborPosition);
    if (blocks_.find(neighborHash) != blocks_.end()) {
        return true;
    }

    return false;
}

boost::unordered_map<unsigned int, boost::shared_ptr<Voxel> > & Chunk::blocks() {
    return blocks_;
}

void Chunk::update() {
    dirty_ = false;
}

void Chunk::render() {
}

void Chunk::invalidate() {
    dirty_ = true;
}

unsigned int Chunk::size() const {
    return size_;
}

bool Chunk::empty() const {
    return empty_;
}

bool Chunk::dirty() const {
    return dirty_;
}

void Chunk::dirty(bool state) {
    dirty_ = state;
}
