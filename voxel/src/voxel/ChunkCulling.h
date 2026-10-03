/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Frustum.h>

#include <glm/vec3.hpp>

/**
 * The box a chunk's geometry can occupy: from its corner to the far corner of its last
 * block, since a block spans one unit from its position.
 *
 * @param origin the chunk's corner, in blocks
 * @param size how many blocks the chunk spans along each axis
 **/
v3d::type::geometry::AABBox chunkBounds(const glm::vec3& origin, float size);

/**
 * @return whether any of a chunk could be in view. A chunk crossing the frustum's edge is,
 *         so nothing at the edge of the screen is dropped
 **/
bool chunkInView(const v3d::type::geometry::Frustum& frustum, const glm::vec3& origin, float size);
