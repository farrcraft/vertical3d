/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>

#include "BRep.h"
#include "Index.h"

namespace v3d::brep {

/**
 * Split an edge at its middle with a new vertex.
 *
 * Both halves of the edge are split, so the pairs stay mutual, and the face on each side gains
 * a corner. A split half edge keeps its index and now ends at the new vertex. The part of it
 * beyond the new vertex is a new half edge, which follows it in its loop. The new parts are
 * appended, so no existing index changes.
 *
 * @param edge either half of the edge
 * @return the new vertex, or nothing when the mesh holds no such half edge
 **/
std::optional<Index> splitEdge(BRep & mesh, Index edge);

/**
 * Split a face in two with a new edge between two of its corners.
 *
 * The face keeps its index and the corners from the second round to the first. The new face
 * holds the corners from the first round to the second. Both keep the face's winding, and each
 * gets its normal from its points. The new parts are appended, so no existing index changes.
 *
 * @return the new face, or nothing when the face is not there, either vertex is not a corner
 *         of it, the two are the same corner, or they are next to each other on it
 **/
std::optional<Index> splitFace(BRep & mesh, Index face, Index first, Index second);

};  // namespace v3d::brep
