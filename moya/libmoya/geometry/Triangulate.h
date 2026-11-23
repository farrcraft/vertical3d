/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

/**
 * The scene's geometry turned into what a ray traversal holds. This is host code: it reads the
 * scene, and what it builds is copied to a device.
 **/
namespace v3d::moya::geometry {

/**
 * A mesh's faces as triangles.
 *
 * A corner is a position in the mesh's indices, so a triangle's points are P[indices[corner]],
 * and a face-varying primvar is read at the corner itself. Each triangle keeps its face's
 * winding.
 **/
struct Triangles {
    /** Three corners per triangle. **/
    std::vector<std::uint32_t> corners;

    /** The face each triangle was cut from, one per triangle. **/
    std::vector<std::uint32_t> faces;

    /** The faces that cover no area, which have no triangles. **/
    std::uint32_t dropped { 0 };
};

/**
 * The triangles of a mesh's faces. A face of more than three vertices is cut by ear clipping
 * within its own plane, so a concave face is cut inside itself. A face of fewer than three
 * vertices, or whose points cover no area, is dropped and counted.
 *
 * A face that is not planar is cut in the plane it is nearest. A face whose edges cross is cut as
 * far as ears can be found, and what remains is cut as a fan.
 *
 * @param points the mesh's P
 * @param vertices the count of each face's vertices
 * @param indices into points, face after face
 * @throws std::invalid_argument when a count is negative, the counts do not add up to the
 *         indices, or an index lies outside the points
 **/
Triangles triangulate(const std::vector<glm::vec3> & points, const std::vector<std::int32_t> & vertices,
    const std::vector<std::int32_t> & indices);

};  // namespace v3d::moya::geometry
