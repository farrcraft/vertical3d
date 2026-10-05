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
 * The position is chunk local, so a chunk's vertices never change when it moves and the
 * origin the pipeline adds back is a push constant rather than a rebuild.
 **/
struct ChunkVertex {
    glm::vec3 position;
    glm::vec2 info;  /**< the face bit, and the block type it was cut from **/
};
