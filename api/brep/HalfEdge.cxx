/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "HalfEdge.h"

namespace v3d::brep {

    const uint64_t INVALID_ID = (1 << 30);

    HalfEdge::HalfEdge() : vertex_(INVALID_ID), pair_(INVALID_ID), next_(INVALID_ID), face_(INVALID_ID) {
    }

    HalfEdge::HalfEdge(uint64_t vert) : vertex_(vert), pair_(INVALID_ID), next_(INVALID_ID), face_(INVALID_ID) {
    }

    HalfEdge::HalfEdge(const HalfEdge& e) {
        *this = e;
    }

    HalfEdge::~HalfEdge() {
    }

    bool HalfEdge::operator == (const HalfEdge& e) {
        return (vertex_ == e.vertex_ &&
            face_ == e.face_ &&
            next_ == e.next_ &&
            pair_ == e.pair_);
    }

    HalfEdge& HalfEdge::operator = (const HalfEdge& e) {
        vertex_ = e.vertex_;
        face_ = e.face_;
        next_ = e.next_;
        pair_ = e.pair_;

        return *this;
    }

    uint64_t HalfEdge::vertex(void) const {
        return vertex_;
    }

    uint64_t HalfEdge::face(void) const {
        return face_;
    }

    uint64_t HalfEdge::pair(void) const {
        return pair_;
    }

    uint64_t HalfEdge::next(void) const {
        return next_;
    }

    void HalfEdge::vertex(uint64_t v) {
        vertex_ = v;
    }

    void HalfEdge::face(uint64_t f) {
        face_ = f;
    }

    void HalfEdge::pair(uint64_t e) {
        pair_ = e;
    }

    void HalfEdge::next(uint64_t e) {
        next_ = e;
    }

};  // namespace v3d::brep
