/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Edge.h"

namespace v3D {

    Edge::Edge() {
    }

    Edge::Edge(const Edge& e) {
        *this = e;
    }

    Edge::Edge(unsigned int prevVertexID, unsigned int nextVertexID) : _prevVertexID(prevVertexID), _nextVertexID(nextVertexID) {
    }

    Edge::~Edge() {
    }

    bool Edge::operator == (const Edge& e) {
        return ((_prevVertexID == e._prevVertexID && _nextVertexID == e._nextVertexID) ||
            (_prevVertexID == e._nextVertexID && _nextVertexID == e._prevVertexID));
    }

    Edge& Edge::operator = (const Edge& e) {
        _selected = e._selected;
        _prevVertexID = e._prevVertexID;
        _nextVertexID = e._nextVertexID;
        _prevFaceID = e._prevFaceID;
        _nextFaceID = e._nextFaceID;
        _prevCWEdgeID = e._prevCWEdgeID;
        _nextCWEdgeID = e._nextCWEdgeID;
        _prevCCWEdgeID = e._prevCCWEdgeID;
        _nextCCWEdgeID = e._nextCCWEdgeID;
        return *this;
    }

    bool Edge::selected(void) const {
        return _selected;
    }

    void Edge::selected(bool sel) {
        _selected = sel;
    }

    unsigned int Edge::prevVertex(void) const {
        return _prevVertexID;
    }

    unsigned int Edge::nextVertex(void) const {
        return _nextVertexID;
    }

    unsigned int Edge::prevFace(void) const {
        return _prevFaceID;
    }

    unsigned int Edge::nextFace(void) const {
        return _nextFaceID;
    }

    unsigned int Edge::prevCWEdge(void) const {
        return _prevCWEdgeID;
    }

    unsigned int Edge::nextCWEdge(void) const {
        return _nextCWEdgeID;
    }

    unsigned int Edge::prevCCWEdge(void) const {
        return _prevCCWEdgeID;
    }

    unsigned int Edge::nextCCWEdge(void) const {
        return _nextCCWEdgeID;
    }

    void Edge::prevVertex(unsigned int vertexID) {
        _prevVertexID = vertexID;
    }

    void Edge::nextVertex(unsigned int vertexID) {
        _nextVertexID = vertexID;
    }

    void Edge::prevFace(unsigned int faceID) {
        _prevFaceID = faceID;
    }

    void Edge::nextFace(unsigned int faceID) {
        _nextFaceID = faceID;
    }

    void Edge::prevCWEdge(unsigned int edgeID) {
        _prevCWEdgeID = edgeID;
    }

    void Edge::nextCWEdge(unsigned int edgeID) {
        _nextCWEdgeID = edgeID;
    }

    void Edge::prevCCWEdge(unsigned int edgeID) {
        _prevCCWEdgeID = edgeID;
    }

    void Edge::nextCCWEdge(unsigned int edgeID) {
        _nextCCWEdgeID = edgeID;
    }

};  // namespace v3D
