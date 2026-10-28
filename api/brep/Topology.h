/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>

#include "BRep.h"
#include "Index.h"

namespace v3d::brep {

/**
 * The half edges of one face, in order.
 *
 * A half edge names the next half edge round its face. A face's geometry is found by
 * following those links, because there is no array of a face's vertices.
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

/** Where an edge starts and where it ends. **/
struct Segment {
    glm::vec3 from{0.0F};
    glm::vec3 to{0.0F};
};

/**
 * The segment one entry of a face loop draws.
 *
 * A half edge names the vertex it ends at, so its segment starts where the one before it in
 * the loop ended.
 *
 * @param loop what faceLoop() returned
 * @param entry which entry of it
 * @return where the segment starts and ends, or nothing when either endpoint cannot be resolved
 **/
std::optional<Segment> loopSegment(const BRep & mesh, const std::vector<Index> & loop, std::size_t entry);

/**
 * Whether a half edge is the one of its pair that stands for the edge.
 *
 * A half edge and its pair are the same edge seen from the two faces that share it, so the
 * lower numbered of the two owns it and the other is skipped. An unpaired edge owns itself.
 **/
bool ownsEdge(const BRep & mesh, Index edge);

/**
 * The half edges that leave a vertex, in index order.
 *
 * A half edge names the vertex it ends at, so the half edge after it in its loop is one that
 * leaves that vertex. This scans every half edge, because nothing keeps a vertex's own edge
 * current.
 **/
std::vector<Index> edgesFrom(const BRep & mesh, Index vertex);

/**
 * The faces that meet at a vertex, each once, in index order. This scans every half edge, as
 * edgesFrom() does.
 **/
std::vector<Index> facesAround(const BRep & mesh, Index vertex);

/**
 * A face's unit normal, found from its points by Newell's method, which holds for a face
 * that is not quite planar. A face's loop runs clockwise seen from outside, so the normal
 * points to the side the loop runs clockwise when seen from.
 *
 * @return zero for a face with no area, or with fewer than three points
 **/
glm::vec3 newellNormal(const BRep & mesh, Index face);

/**
 * The same normal, for a face given as its points in loop order.
 **/
glm::vec3 newellNormal(const std::vector<glm::vec3> & points);

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
