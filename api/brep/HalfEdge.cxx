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

HalfEdge::HalfEdge(const HalfEdge& other) {
    *this = other;
}

HalfEdge::~HalfEdge() {
}

bool HalfEdge::operator == (const HalfEdge& other) const {
    return (vertex_ == other.vertex_ &&
        face_ == other.face_ &&
        next_ == other.next_ &&
        pair_ == other.pair_);
}

HalfEdge& HalfEdge::operator = (const HalfEdge& other) {
    vertex_ = other.vertex_;
    face_ = other.face_;
    next_ = other.next_;
    pair_ = other.pair_;
    selected_ = other.selected_;

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

void HalfEdge::face(Index faceIndex) {
    face_ = faceIndex;
}

void HalfEdge::pair(Index pairIndex) {
    pair_ = pairIndex;
}

void HalfEdge::next(Index nextIndex) {
    next_ = nextIndex;
}

};  // namespace v3d::brep
