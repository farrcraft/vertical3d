/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Voxel.h"

#include <glm/vec3.hpp>

/**
 * Which block in a neighbouring chunk sits against a block's face.
 *
 * A block on the edge of a chunk has one face its own chunk cannot answer for, and the
 * block behind that face is the far edge of the chunk next door - the block at zero when
 * looking out the high side, and the block at chunkSize - 1 when looking out the low side.
 * Getting that backwards keeps faces that should be culled and culls faces that should be
 * kept, and only shows up as seams between chunks.
 **/
struct Neighbour {
    Neighbour();

    bool crosses;      /**< whether the face lies on a seam with a chunk that could exist **/
    glm::ivec3 chunk;  /**< which chunk to look in **/
    glm::ivec3 block;  /**< the block in it whose opposing face would hide this one **/
};

/**
 * @param face the face of the block being tested
 * @param block the block's position within its chunk
 * @param chunk the chunk's position in the world, in chunks
 * @param chunkSize how many blocks a chunk spans in one dimension
 * @return where to look, or crosses false when the face is not on a seam or the chunk
 *         beyond it would be outside the world
 **/
Neighbour neighbourAcrossSeam(Voxel::BlockFace face, const glm::ivec3& block, const glm::ivec3& chunk, int chunkSize);
