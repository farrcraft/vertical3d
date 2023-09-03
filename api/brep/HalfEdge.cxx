/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "HalfEdge.h"

namespace v3d::brep {

    const unsigned int INVALID_ID = (1 << 31);

    HalfEdge::HalfEdge() : vertex_(INVALID_ID), pair_(INVALID_ID), next_(INVALID_ID), face_(INVALID_ID) {
    }

    HalfEdge::HalfEdge(unsigned int vert) : vertex_(vert), pair_(INVALID_ID), next_(INVALID_ID), face_(INVALID_ID) {
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

    unsigned int HalfEdge::vertex(void) const {
        return vertex_;
    }

    unsigned int HalfEdge::face(void) const {
        return face_;
    }

    unsigned int HalfEdge::pair(void) const {
        return pair_;
    }

    unsigned int HalfEdge::next(void) const {
        return next_;
    }

    void HalfEdge::vertex(unsigned int v) {
        vertex_ = v;
    }

    void HalfEdge::face(unsigned int f) {
        face_ = f;
    }

    void HalfEdge::pair(unsigned int e) {
        pair_ = e;
    }

    void HalfEdge::next(unsigned int e) {
        next_ = e;
    }

};  // namespace v3d::brep
