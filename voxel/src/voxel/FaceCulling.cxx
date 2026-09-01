/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FaceCulling.h"

Neighbour::Neighbour() :
    crosses(false),
    chunk(0, 0, 0),
    block(0, 0, 0) {
}

Neighbour neighbourAcrossSeam(Voxel::BlockFace face, const glm::ivec3& block, const glm::ivec3& chunk, int chunkSize) {
    const int high = chunkSize - 1;

    Neighbour neighbour;
    neighbour.chunk = chunk;
    neighbour.block = block;

    switch (face) {
        case Voxel::BLOCK_FACE_LEFT:
            if (block.x == 0 && chunk.x > 0) {
                neighbour.chunk.x -= 1;
                neighbour.block.x = high;
                neighbour.crosses = true;
            }
            break;
        case Voxel::BLOCK_FACE_RIGHT:
            if (block.x == high) {
                neighbour.chunk.x += 1;
                neighbour.block.x = 0;
                neighbour.crosses = true;
            }
            break;
        case Voxel::BLOCK_FACE_BACK:
            if (block.z == 0 && chunk.z > 0) {
                neighbour.chunk.z -= 1;
                neighbour.block.z = high;
                neighbour.crosses = true;
            }
            break;
        case Voxel::BLOCK_FACE_FRONT:
            if (block.z == high) {
                neighbour.chunk.z += 1;
                neighbour.block.z = 0;
                neighbour.crosses = true;
            }
            break;
        case Voxel::BLOCK_FACE_TOP:
            if (block.y == high) {
                neighbour.chunk.y += 1;
                neighbour.block.y = 0;
                neighbour.crosses = true;
            }
            break;
        case Voxel::BLOCK_FACE_BOTTOM:
            if (block.y == 0 && chunk.y > 0) {
                neighbour.chunk.y -= 1;
                neighbour.block.y = high;
                neighbour.crosses = true;
            }
            break;
        default:
            break;
    }

    return neighbour;
}
