/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ChunkCulling.h"

v3d::type::geometry::AABBox chunkBounds(const glm::vec3& origin, float size) {
    v3d::type::geometry::AABBox bounds;
    bounds.extents(origin, origin + glm::vec3(size));
    return bounds;
}

bool chunkInView(const v3d::type::geometry::Frustum& frustum, const glm::vec3& origin, float size) {
    return frustum.intersect(chunkBounds(origin, size)) != v3d::type::geometry::Frustum::OUTSIDE;
}
