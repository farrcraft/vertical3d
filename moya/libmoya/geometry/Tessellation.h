/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::geometry {

/**
 * A subdivision surface cut into triangles. Each place is a point on the limit surface, named by
 * the patch face it lies on and its position there. A face of four vertices is one patch face. A
 * Catmull-Clark or bilinear face of n other than four is n patch faces, one at each of its
 * corners, and a Loop face is one triangular patch face.
 *
 * Places are not shared between patch faces, so a face-varying value can differ on each side of
 * an edge. A place on an edge or a corner that several patch faces reach names the place whose
 * point it takes, so that the surface has no cracks.
 **/
struct Tessellation {
    /** The patch face of each place. **/
    std::vector<std::uint32_t> patches;

    /** The position of each place on its patch face, from 0 to 1 on each side. **/
    std::vector<glm::vec2> positions;

    /**
     * The place whose point each place takes: the first place cut where it lies, which takes its
     * own. It is empty for places that take their own points.
     **/
    std::vector<std::uint32_t> shared;

    /** The face of the control mesh each place lies on. **/
    std::vector<std::uint32_t> placeFaces;

    /** Three places per triangle, in the winding of the face they were cut from. **/
    std::vector<std::uint32_t> corners;

    /** The face of the control mesh each triangle was cut from, one per triangle. **/
    std::vector<std::uint32_t> faces;
};

};  // namespace v3d::moya::geometry
