/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Index.h"

namespace v3d::brep {

/**
 * One edge of a winged-edge mesh.
 *
 * Unlike a HalfEdge, an edge is shared: it names the vertex at each end, the face on
 * each side, and the four edges it meets at those ends - the wings. That is what lets a
 * traversal turn either way from an edge without a paired twin.
 **/
class Edge {
 public:
    Edge();
    Edge(const Edge & e);
    Edge(Index prevVertex, Index nextVertex);
    ~Edge();

    // const, because C++20's reversed candidate for a non-const operator== makes every
    // a == b ambiguous with the b == a it synthesizes
    bool operator == (const Edge & e) const;
    Edge & operator = (const Edge & e);

    /**
     * Whether this component is selected. Selection is per component, not per mesh, so
     * a vertex, an edge and a face each carry their own.
     **/
    bool selected(void) const noexcept;
    void selected(bool sel) noexcept;

    Index prevVertex(void) const;
    Index nextVertex(void) const;
    Index prevFace(void) const;
    Index nextFace(void) const;
    Index prevCWEdge(void) const;
    Index nextCWEdge(void) const;
    Index prevCCWEdge(void) const;
    Index nextCCWEdge(void) const;
    void prevVertex(Index vertex);
    void nextVertex(Index vertex);
    void prevFace(Index face);
    void nextFace(Index face);
    void prevCWEdge(Index edge);
    void nextCWEdge(Index edge);
    void prevCCWEdge(Index edge);
    void nextCCWEdge(Index edge);

 private:
    Index prevVertex_;  // vertices
    Index nextVertex_;
    Index prevFace_;  // adjacent faces
    Index nextFace_;
    Index prevCWEdge_;  // wings
    Index nextCWEdge_;
    Index prevCCWEdge_;
    Index nextCCWEdge_;
    bool selected_;
};

};  // namespace v3d::brep
