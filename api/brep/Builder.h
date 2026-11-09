/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "BuildOptions.h"
#include "Built.h"
#include "Polygons.h"

namespace v3d::brep {

/**
 * Build a mesh from polygons.
 *
 * Points at exactly the same position become one vertex, and -0 is the same position as 0. Two
 * corners in a row at one vertex count once. Faces that meet along an edge are paired across
 * it, and an edge with only one face is the mesh's boundary. Each face's normal is found from
 * its points. The time taken grows in proportion to the number of corners, apart from sorting
 * the pairs of triangles that could be joined.
 *
 * The input is refused when a count or an index is out of range, or a point is not finite. It
 * is also refused when a face visits one vertex twice, or two faces run along one edge in the
 * same direction. The last
 * happens where three or more faces meet at an edge, or where neighbouring faces disagree about
 * which side is outside.
 **/
Built build(const Polygons& polygons, const BuildOptions& options);

};  // namespace v3d::brep
