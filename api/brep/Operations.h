/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <vector>

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

/**
 * Extrude faces: each face moves onto copies of the corners on its outline, and a ring of new
 * quads joins the copies to the corners left behind.
 *
 * Faces that share an edge extrude as one region. The edges between them stay, and only the
 * region's outline gains a ring. A corner inside a region is not copied, because no face outside
 * the region uses it. Two regions that touch only at a corner each get their own copy of it.
 *
 * The copies sit where the corners are, so the faces have not moved and the ring's quads have no
 * area and a zero normal. A caller moves the faces after. Each face keeps its index and its half
 * edges. The new parts are appended, so no existing index changes.
 *
 * @return the ring's new faces, or nothing when a face is not there
 **/
std::optional<std::vector<Index>> extrudeFaces(BRep & mesh, const std::vector<Index> & faces);

/**
 * Inset faces: a smaller copy of each face inside it, joined to its outline by a ring of quads.
 *
 * Each face is inset on its own, so two faces that share an edge keep it between their rings.
 * Each face keeps its index and becomes the copy. Each corner of the copy is a fraction of the
 * way from the face's corner to the face's centre. The ring's quads lie in the face's plane, and
 * each gets its normal from its points. The new parts are appended, so no existing index changes.
 *
 * @param fraction how far toward the centre each corner moves, from 0 to 1
 * @return the rings' new faces, or nothing when a face is not there
 **/
std::optional<std::vector<Index>> insetFaces(BRep & mesh, const std::vector<Index> & faces, float fraction);

/**
 * Merge vertices into one at their centre.
 *
 * An edge between two merged vertices has no length, and goes. A face left with fewer than three
 * corners goes, and so do the vertices nothing uses after. Edges that now run between the same
 * two vertices are paired, so a merge can join two open boundaries.
 *
 * The arrays are compacted, so the parts after a removed one are renumbered. The merged vertex
 * keeps its selection, and every other part keeps its own.
 *
 * A merge is refused, and changes nothing, when it would leave an edge shared by more than two
 * faces, a face that meets itself, or nothing at the merged point.
 *
 * @return the merged vertex, or nothing when the merge is refused, fewer than two vertices are
 *         named, or a vertex is not there
 **/
std::optional<Index> mergeVertices(BRep & mesh, const std::vector<Index> & vertices);

};  // namespace v3d::brep
