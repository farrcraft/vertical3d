/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/**
 * One vertex of a chunk mesh, in the layout shaders/voxel.vert declares.
 *
 * The position is chunk local. The chunk's origin is added back in the shader from a push
 * constant, so moving a chunk does not change its vertices.
 **/
struct ChunkVertex {
    glm::vec3 position;
    glm::vec2 info;  /**< the face bit, and the block type it was cut from **/
};
