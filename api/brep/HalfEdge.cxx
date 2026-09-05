/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "HalfEdge.h"

namespace v3d::brep {

HalfEdge::HalfEdge() : vertex_(INVALID_ID), pair_(INVALID_ID), next_(INVALID_ID), face_(INVALID_ID), selected_(false) {
}

HalfEdge::HalfEdge(Index vert) : vertex_(vert), pair_(INVALID_ID), next_(INVALID_ID), face_(INVALID_ID), selected_(false) {
}

HalfEdge::HalfEdge(const HalfEdge& e) {
    *this = e;
}

HalfEdge::~HalfEdge() {
}

bool HalfEdge::operator == (const HalfEdge& e) const {
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
    selected_ = e.selected_;

    return *this;
}

bool HalfEdge::selected(void) const noexcept {
    return selected_;
}

void HalfEdge::selected(bool sel) noexcept {
    selected_ = sel;
}

Index HalfEdge::vertex(void) const {
    return vertex_;
}

Index HalfEdge::face(void) const {
    return face_;
}

Index HalfEdge::pair(void) const {
    return pair_;
}

Index HalfEdge::next(void) const {
    return next_;
}

void HalfEdge::vertex(Index v) {
    vertex_ = v;
}

void HalfEdge::face(Index f) {
    face_ = f;
}

void HalfEdge::pair(Index e) {
    pair_ = e;
}

void HalfEdge::next(Index e) {
    next_ = e;
}

};  // namespace v3d::brep
