/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>
#include <algorithm>
#include <cassert>

#include "WingedEdgeBRep.h"

namespace v3D {

const unsigned int WingedEdgeBRep::INVALID_ID = (1 << 31);


WingedEdgeBRep::edge_iterator::edge_iterator() : _brep(0), _firstEdgeID(INVALID_ID), _edge(0) {
}

WingedEdgeBRep::edge_iterator::edge_iterator(WingedEdgeBRep * brep, unsigned int faceID) {
    reset(brep, faceID);
}

WingedEdgeBRep::edge_iterator::~edge_iterator() {
}

void WingedEdgeBRep::edge_iterator::reset(WingedEdgeBRep * brep, unsigned int faceID) {
    _brep = brep;
    Face * face = _brep->face(faceID);
    if (!face) {
        _edge = 0;
        return;
    }
    _firstEdgeID = face->edge();
    _edge = _brep->edge(_firstEdgeID);

    if (_edge->nextFace() == faceID)  // ccw
        _winding = true;
    else
        _winding = false;
}

Edge * WingedEdgeBRep::edge_iterator::operator * () {
    return _edge;
}

WingedEdgeBRep::edge_iterator WingedEdgeBRep::edge_iterator::operator++ (int) {
    if (!_edge)
        return *this;
    unsigned int nextEdgeID;
    if (_winding)
        nextEdgeID = _edge->nextCCWEdge();
    else
        nextEdgeID = _edge->prevCWEdge();
    if (nextEdgeID == _firstEdgeID) {
        _edge = 0;
        return *this;
    }
    _edge = _brep->edge(nextEdgeID);

    return *this;
}

WingedEdgeBRep * WingedEdgeBRep::edge_iterator::brep(void) const {
    return _brep;
}

WingedEdgeBRep::vertex_iterator::vertex_iterator() : _nextVertex(false) {
}

WingedEdgeBRep::vertex_iterator::vertex_iterator(WingedEdgeBRep * brep, unsigned int faceID) : _nextVertex(false) {
    reset(brep, faceID);
}

WingedEdgeBRep::vertex_iterator::~vertex_iterator() {
}

Vertex * WingedEdgeBRep::vertex_iterator::operator * () {
    if ((*_iterator) == 0)
        return 0;
    if (_nextVertex)
        return (_iterator.brep()->vertex((*_iterator)->nextVertex()));
    else
        return (_iterator.brep()->vertex((*_iterator)->prevVertex()));
}

WingedEdgeBRep::vertex_iterator WingedEdgeBRep::vertex_iterator::operator++ (int) {
    if ((*_iterator) == 0)
        return *this;
    if (_nextVertex)
        _iterator++;
    _nextVertex = !_nextVertex;
    return *this;
}

void WingedEdgeBRep::vertex_iterator::reset(WingedEdgeBRep * brep, unsigned int faceID) {
    _iterator.reset(brep, faceID);
}


WingedEdgeBRep::WingedEdgeBRep() : _selected(false) {
}

WingedEdgeBRep::~WingedEdgeBRep() {
}

Vector3 WingedEdgeBRep::center(unsigned int faceID) {
    float nverts = 0.0;
    Vector3 mid(0.0, 0.0, 0.0);

    vertex_iterator it(this, faceID);
    Vertex * vert;
    // get edge vertices
    Vector3 pt;
    for (; *it != 0; it++) {
        vert = *it;
        pt = vert->point();
        mid += pt;
        nverts += 1.0;
    }

    mid /= nverts;

    return mid;
}

void WingedEdgeBRep::faceUV(unsigned int faceID, Vector3 & u, Vector3 & v) {
    edge_iterator it(this, faceID);
    if (*it == 0)
        return;

    Edge * edge = *it;
    if (!edge)
        return;

    Vector3 norm;

    u = vertex(edge->nextVertex())->point() - vertex(edge->prevVertex())->point();
    it++;
    edge = *it;
    v = vertex(edge->nextVertex())->point() - vertex(edge->prevVertex())->point();

    norm = u.cross(v);
    norm.normalize();
    v = u.cross(norm);
    u = v.cross(norm);
    u.normalize();
    v.normalize();
}

unsigned int WingedEdgeBRep::addVertex(const Vector3 & v) {
    unsigned int index = 0;
    for (; index < _vertices.size(); index++) {
        if (_vertices[index] == v)
            return index;
    }
    Vertex vert(v);
    _vertices.push_back(vert);
    return (_vertices.size() - 1);
}

unsigned int WingedEdgeBRep::addEdge(unsigned int leftVertex, unsigned int rightVertex) {
    Edge e(leftVertex, rightVertex);
    unsigned int index = 0;
    for (; index < _edges.size(); index++) {
    // if ((_edges[index].prevVertex() == leftVertex) && (_edges[index].nextVertex() == rightVertex))
        if (_edges[index] == e) {
            return index;
        }
    }
    _edges.push_back(e);
    return (_edges.size() - 1);
}

void WingedEdgeBRep::addFace(const std::vector<Vector3> & vertices, const Vector3 & normal, bool winding) {
    // add vertices
    std::vector<unsigned int> indices;
    unsigned int vertexID;
    std::vector<Vector3>::const_iterator it = vertices.begin();
    for (; it != vertices.end(); it++) {
        vertexID = addVertex(*it);
        std::cerr << "vertex " << vertexID << " = " << *it << std::endl;
        indices.push_back(vertexID);
    }
    // add edges
    unsigned int index = 0;
    unsigned int leftVertex, rightVertex;
    unsigned int edgeID;
    std::vector<unsigned int> edges;
    for (; index < indices.size(); index++) {
        leftVertex = indices[index];
        if (index == (indices.size() - 1))
            rightVertex = indices[0];
        else
            rightVertex = indices[index + 1];

        edgeID = addEdge(leftVertex, rightVertex);
        std::cerr << "add edge " << leftVertex << ", " << rightVertex << " = " << edgeID << std::endl;
        edges.push_back(edgeID);
    }

    // add face
    unsigned int faceID;
    Face f(normal, edges[0]);
    _faces.push_back(f);
    faceID = (_faces.size() - 1);

    std::cerr << "building edges" << std::endl;
    // finish constructing edges
    Edge * ep;
    for (index = 0; index < edges.size(); index++) {
        ep = edge(edges[index]);
        assert(ep != 0);
        // ccw
        if (winding) {
            ep->nextFace(faceID);
            if (index == (edges.size() - 1))
                ep->nextCCWEdge(edges[0]);
            else
                ep->nextCCWEdge(edges[index + 1]);
            if (index == 0)
                ep->nextCWEdge(edges[edges.size() - 1]);
            else
                ep->nextCWEdge(edges[index - 1]);
            std::cerr << "ccw winding edgeID = " << edges[index]  << " next CCW Edge = " << ep->nextCCWEdge() << " next CW Edge = " << ep->nextCWEdge() << std::endl;
        } else {  // cw
            ep->prevFace(faceID);
            if (index == (edges.size() - 1))
                ep->prevCWEdge(edges[0]);
            else
                ep->prevCWEdge(edges[index + 1]);
            if (index == 0)
                ep->prevCCWEdge(edges[edges.size() - 1]);
            else
                ep->prevCCWEdge(edges[index - 1]);

            std::cerr << "cw winding prev CCW Edge = " << ep->prevCCWEdge() << " prev CW Edge = " << ep->prevCWEdge() << std::endl;
        }
    }
    std::cerr << "done building edges" << std::endl;
}

Edge * WingedEdgeBRep::edge(unsigned int edgeID) {
    if (edgeID < _edges.size())
        return &_edges[edgeID];
    return 0;
}

Face * WingedEdgeBRep::face(unsigned int faceID) {
    if (faceID < _faces.size())
        return &_faces[faceID];
    return 0;
}

Vertex * WingedEdgeBRep::vertex(unsigned int vertexID) {
    if (vertexID < _vertices.size())
        return &_vertices[vertexID];
    return 0;
}

unsigned int WingedEdgeBRep::vertexCount(void) const {
    return _vertices.size();
}

unsigned int WingedEdgeBRep::edgeCount(void) const {
    return _edges.size();
}

unsigned int WingedEdgeBRep::faceCount(void) const {
    return _faces.size();
}

unsigned int WingedEdgeBRep::addVertex(const Vertex & v) {
    _vertices.push_back(v);
    return (_vertices.size() - 1);
}

unsigned int WingedEdgeBRep::addEdge(const Edge & e) {
    _edges.push_back(e);
    return (_edges.size() - 1);
}

unsigned int WingedEdgeBRep::addFace(const Face & f) {
    _faces.push_back(f);
    return (_faces.size() - 1);
}

void WingedEdgeBRep::splitEdge(unsigned int edgeID, const Vector3 & point) {
    unsigned int vertexID;
    _vertices.push_back(point);
    vertexID = _vertices.size() - 1;

    Edge newEdge = _edges[edgeID];
    // edge goes from PVT to point
    // newEdge goes from point to NVT
    _edges[edgeID].nextVertex(vertexID);
    newEdge.prevVertex(vertexID);

    unsigned newEdgeID;
    _edges.push_back(newEdge);
    newEdgeID = _edges.size() - 1;

    _edges[edgeID].nextCCWEdge(newEdgeID);
    _edges[edgeID].prevCWEdge(newEdgeID);

    _edges[newEdgeID].nextCWEdge(edgeID);
    _edges[newEdgeID].prevCCWEdge(edgeID);
}

// calculate object-space bounds of mesh
AABBox WingedEdgeBRep::bound(void) const {
    AABBox extents;
    if (_vertices.size() == 0)
        return extents;
    Vector3 min, max;
    min = _vertices[0].point();
    max = min;
    Vector3 vt;
    for (unsigned int index = 1; index < _vertices.size(); index++) {
        vt = _vertices[index].point();
        if (vt[0] < min[0])
            min[0] = vt[0];
        if (vt[1] < min[1])
            min[1] = vt[1];
        if (vt[2] < min[2])
            min[2] = vt[2];

        if (vt[0] > max[0])
            max[0] = vt[0];
        if (vt[1] > max[1])
            max[1] = vt[1];
        if (vt[2] > max[2])
            max[2] = vt[2];
    }
    extents.extents(min, max);
    return extents;
}

bool WingedEdgeBRep::selected(void) const {
    return _selected;
}

void WingedEdgeBRep::selected(bool sel) {
    _selected = sel;
    if (!_selected) {
        // make sure components are unselected also
        unsigned int vertex_count = 0;
        for (; vertex_count < _vertices.size(); vertex_count++) {
            _vertices[vertex_count].selected(false);
        }
        unsigned int face_count = 0;
        for (; face_count < _faces.size(); face_count++) {
            _faces[face_count].selected(false);
        }
        unsigned int edge_count = 0;
        for (; edge_count < _edges.size(); edge_count++) {
            _edges[edge_count].selected(false);
        }
    }
}

Vector3 WingedEdgeBRep::translation(void) const {
    return DAG::Transform::translation();
}

void WingedEdgeBRep::translation(const Vector3 & t) {
    bool component_translate = false;

    // translate selected vertices
    unsigned int vertex_count = 0;
    for (; vertex_count < _vertices.size(); vertex_count++) {
        if (_vertices[vertex_count].selected()) {
            component_translate = true;
            _vertices[vertex_count].point(_vertices[vertex_count].point() + t);
        }
    }

    // translate selected faces
    unsigned int face_count = 0;
    for (; face_count < _faces.size(); face_count++) {
        if (_faces[face_count].selected()) {
            component_translate = true;
            vertex_iterator it(this, face_count);
            for (; *it != 0; it++) {
                Vertex * vert = *it;
                vert->point(vert->point() + t);
            }
        }
    }

    // translate selected edges
    unsigned int edge_count = 0;
    for (; edge_count < _edges.size(); edge_count++) {
        if (_edges[edge_count].selected()) {
            component_translate = true;
            _vertices[_edges[edge_count].prevVertex()].point(_vertices[_edges[edge_count].prevVertex()].point() + t);
            _vertices[_edges[edge_count].nextVertex()].point(_vertices[_edges[edge_count].nextVertex()].point() + t);
        }
    }

    // translate object
    if (!component_translate)
        DAG::Transform::translation(t);
}

};  // namespace v3D
