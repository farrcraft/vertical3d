/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cstdint>

namespace v3d::brep {

    /**
     * The id a half edge, face or vertex reference carries when it points at nothing.
     * BRep::INVALID_ID is the same value - these were two different constants, 1 << 30
     * here and 1 << 31 there, so an unpaired edge never compared equal to the sentinel
     * BRep tested it against.
     **/
    constexpr uint64_t INVALID_ID = (1ull << 31);

    class HalfEdge {
     public:
        HalfEdge();
        explicit HalfEdge(uint64_t vert);
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
        bool selected_;
    };

};  // namespace v3d::brep
