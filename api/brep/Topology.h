/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <vector>

#include <glm/vec3.hpp>

#include "BRep.h"
#include "Index.h"

namespace v3d::brep {

/**
 * The half edges of one face, in order.
 *
 * A half edge names the next edge round its face, so a face's geometry is reached by walking
 * the chain - there is no array of a face's vertices to index into.
 *
 * Each half edge appears at most once, so the result never holds more entries than the
 * mesh has half edges. An unfinished modelling operation can leave a next chain that does
 * not return to the face's first edge. For such a chain the result is the distinct edges
 * walked, in order, until the chain repeats an edge, names INVALID_ID or names an edge the
 * mesh does not hold. That result is an open path rather than a ring.
 *
 * @return the half edges, or nothing if the face does not exist
 **/
std::vector<Index> faceLoop(const BRep & mesh, Index face);

/**
 * The segment one entry of a face loop draws.
 *
 * A half edge names the vertex it ends at, so its segment starts where the one before it in
 * the loop ended.
 *
 * @param loop what faceLoop() returned
 * @param entry which entry of it
 * @param from where the segment starts - may be null
 * @param to where it ends - may be null
 * @return whether both endpoints could be resolved
 **/
bool loopSegment(const BRep & mesh, const std::vector<Index> & loop, std::size_t entry, glm::vec3* from, glm::vec3* to);

/**
 * Whether a half edge is the one of its pair that stands for the edge.
 *
 * A half edge and its pair are the same edge seen from the two faces that share it, so the
 * lower numbered of the two owns it and the other is skipped. An unpaired edge owns itself.
 **/
bool ownsEdge(const BRep & mesh, Index edge);

/**
 * Whether the edge a half edge belongs to is selected: the two halves are one edge to a
 * selection, so either being selected is enough.
 **/
bool edgeSelected(const BRep & mesh, Index edge);

/**
 * The mid point of a face: the average of the vertices its loop ends at.
 **/
glm::vec3 center(const BRep & mesh, Index face);

/**
 * Two unit vectors in the plane of a face, square to each other.
 *
 * They are taken from the face's first edge that has a length, and the first edge after it that
 * is not parallel to it. For a face whose first two edges turn a corner, that is those two. u
 * runs along the first edge, and v is square to it.
 *
 * Left as they were for a face of fewer than two edges, or one whose vertices all lie on a line.
 **/
void faceUV(const BRep & mesh, Index face, glm::vec3* u, glm::vec3* v);

};  // namespace v3d::brep
