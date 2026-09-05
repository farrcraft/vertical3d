/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Index.h"

namespace v3d::brep {

class HalfEdge {
 public:
    HalfEdge();
    explicit HalfEdge(Index vert);
    explicit HalfEdge(const HalfEdge & e);
    ~HalfEdge();

    // const, because C++20's reversed candidate for a non-const operator== makes every
    // a == b ambiguous with the b == a it synthesizes
    bool operator == (const HalfEdge & e) const;
    HalfEdge & operator = (const HalfEdge & e);

    /**
     * Whether this component is selected. Selection is per component, not per mesh, so
     * a vertex, an edge and a face each carry their own.
     **/
    bool selected(void) const noexcept;
    void selected(bool sel) noexcept;

    Index vertex(void) const;
    Index face(void) const;
    Index pair(void) const;
    Index next(void) const;

    void vertex(Index vert);
    void face(Index f);
    void pair(Index e);
    void next(Index e);

 private:
    Index vertex_;  // vertex at end of half edge
    Index face_;  // face to left of edge
    Index pair_;  // symetric half edge
    Index next_;  // next CCW half edge
    bool selected_;
};

};  // namespace v3d::brep
