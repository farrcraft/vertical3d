/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Edge.h"

namespace v3d::brep {

    Edge::Edge() :
        prevVertex_(INVALID_ID), nextVertex_(INVALID_ID),
        prevFace_(INVALID_ID), nextFace_(INVALID_ID),
        prevCWEdge_(INVALID_ID), nextCWEdge_(INVALID_ID),
        prevCCWEdge_(INVALID_ID), nextCCWEdge_(INVALID_ID),
        selected_(false) {
    }

    Edge::Edge(const Edge& e) {
        *this = e;
    }

    Edge::Edge(Index prevVertex, Index nextVertex) :
        prevVertex_(prevVertex), nextVertex_(nextVertex),
        prevFace_(INVALID_ID), nextFace_(INVALID_ID),
        prevCWEdge_(INVALID_ID), nextCWEdge_(INVALID_ID),
        prevCCWEdge_(INVALID_ID), nextCCWEdge_(INVALID_ID),
        selected_(false) {
    }

    Edge::~Edge() {
    }

    bool Edge::operator == (const Edge& e) const {
        return ((prevVertex_ == e.prevVertex_ && nextVertex_ == e.nextVertex_) ||
            (prevVertex_ == e.nextVertex_ && nextVertex_ == e.prevVertex_));
    }

    Edge& Edge::operator = (const Edge& e) {
        selected_ = e.selected_;
        prevVertex_ = e.prevVertex_;
        nextVertex_ = e.nextVertex_;
        prevFace_ = e.prevFace_;
        nextFace_ = e.nextFace_;
        prevCWEdge_ = e.prevCWEdge_;
        nextCWEdge_ = e.nextCWEdge_;
        prevCCWEdge_ = e.prevCCWEdge_;
        nextCCWEdge_ = e.nextCCWEdge_;
        return *this;
    }

    bool Edge::selected(void) const noexcept {
        return selected_;
    }

    void Edge::selected(bool sel) noexcept {
        selected_ = sel;
    }

    Index Edge::prevVertex(void) const {
        return prevVertex_;
    }

    Index Edge::nextVertex(void) const {
        return nextVertex_;
    }

    Index Edge::prevFace(void) const {
        return prevFace_;
    }

    Index Edge::nextFace(void) const {
        return nextFace_;
    }

    Index Edge::prevCWEdge(void) const {
        return prevCWEdge_;
    }

    Index Edge::nextCWEdge(void) const {
        return nextCWEdge_;
    }

    Index Edge::prevCCWEdge(void) const {
        return prevCCWEdge_;
    }

    Index Edge::nextCCWEdge(void) const {
        return nextCCWEdge_;
    }

    void Edge::prevVertex(Index vertex) {
        prevVertex_ = vertex;
    }

    void Edge::nextVertex(Index vertex) {
        nextVertex_ = vertex;
    }

    void Edge::prevFace(Index face) {
        prevFace_ = face;
    }

    void Edge::nextFace(Index face) {
        nextFace_ = face;
    }

    void Edge::prevCWEdge(Index edge) {
        prevCWEdge_ = edge;
    }

    void Edge::nextCWEdge(Index edge) {
        nextCWEdge_ = edge;
    }

    void Edge::prevCCWEdge(Index edge) {
        prevCCWEdge_ = edge;
    }

    void Edge::nextCCWEdge(Index edge) {
        nextCCWEdge_ = edge;
    }

};  // namespace v3d::brep
