/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cstdint>

namespace v3d::brep {

    class HalfEdge {
     public:
        HalfEdge();
        explicit HalfEdge(uint64_t vert);
        explicit HalfEdge(const HalfEdge & e);
        ~HalfEdge();

        bool operator == (const HalfEdge & e);
        HalfEdge & operator = (const HalfEdge & e);

        uint64_t vertex(void) const;
        uint64_t face(void) const;
        uint64_t pair(void) const;
        uint64_t next(void) const;

        void vertex(uint64_t vert);
        void face(uint64_t f);
        void pair(uint64_t e);
        void next(uint64_t e);

     private:
        uint64_t vertex_;  // vertex at end of half edge
        uint64_t face_;  // face to left of edge
        uint64_t pair_;  // symetric half edge
        uint64_t next_;  // next CCW half edge
    };

};  // namespace v3d::brep
