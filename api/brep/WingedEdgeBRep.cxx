/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>
#include <cassert>
#include <vector>

#include <glm/common.hpp>

#include "WingedEdgeBRep.h"

namespace v3d::brep {

const Index WingedEdgeBRep::INVALID_ID = v3d::brep::INVALID_ID;


WingedEdgeBRep::edge_iterator::edge_iterator() : brep_(0), firstEdge_(INVALID_ID), edge_(0) {
}

WingedEdgeBRep::edge_iterator::edge_iterator(WingedEdgeBRep * brep, Index faceID) {
    reset(brep, faceID);
}

WingedEdgeBRep::edge_iterator::~edge_iterator() {
}

void WingedEdgeBRep::edge_iterator::reset(WingedEdgeBRep * brep, Index faceID) {
    brep_ = brep;
    Face * face = brep_->face(faceID);
    if (!face) {
        edge_ = 0;
        return;
    }
    firstEdge_ = face->edge();
    edge_ = brep_->edge(firstEdge_);

    winding_ = edge_->nextFace() == faceID;  // ccw
}

Edge * WingedEdgeBRep::edge_iterator::operator * () {
    return edge_;
}

WingedEdgeBRep::edge_iterator WingedEdgeBRep::edge_iterator::operator++ (int) {
    if (!edge_)
        return *this;
    Index nextEdgeID;
    if (winding_)
        nextEdgeID = edge_->nextCCWEdge();
    else
        nextEdgeID = edge_->prevCWEdge();
    if (nextEdgeID == firstEdge_) {
        edge_ = 0;
        return *this;
    }
    edge_ = brep_->edge(nextEdgeID);

    return *this;
}

WingedEdgeBRep * WingedEdgeBRep::edge_iterator::brep(void) const {
    return brep_;
}

WingedEdgeBRep::vertex_iterator::vertex_iterator() : nextVertex_(false) {
}

WingedEdgeBRep::vertex_iterator::vertex_iterator(WingedEdgeBRep * brep, Index faceID) : nextVertex_(false) {
    reset(brep, faceID);
}

WingedEdgeBRep::vertex_iterator::~vertex_iterator() {
}

Vertex * WingedEdgeBRep::vertex_iterator::operator * () {
    if ((*iterator_) == 0)
        return 0;
    if (nextVertex_)
        return (iterator_.brep()->vertex((*iterator_)->nextVertex()));
    return (iterator_.brep()->vertex((*iterator_)->prevVertex()));
}

WingedEdgeBRep::vertex_iterator WingedEdgeBRep::vertex_iterator::operator++ (int) {
    if ((*iterator_) == 0)
        return *this;
    if (nextVertex_)
        iterator_++;
    nextVertex_ = !nextVertex_;
    return *this;
}

void WingedEdgeBRep::vertex_iterator::reset(WingedEdgeBRep * brep, Index faceID) {
    iterator_.reset(brep, faceID);
}


WingedEdgeBRep::WingedEdgeBRep() : selected_(false) {
}

WingedEdgeBRep::~WingedEdgeBRep() {
}

glm::vec3 WingedEdgeBRep::center(Index faceID) {
    float nverts = 0.0;
    glm::vec3 mid(0.0, 0.0, 0.0);

    vertex_iterator it(this, faceID);
    Vertex * vert;
    // get edge vertices
    glm::vec3 pt;
    for (; *it != 0; it++) {
        vert = *it;
        pt = vert->point();
        mid += pt;
        nverts += 1.0;
    }

    mid /= nverts;

    return mid;
}

void WingedEdgeBRep::faceUV(Index faceID, glm::vec3 * u, glm::vec3 * v) {
    edge_iterator it(this, faceID);
    if (*it == 0)
        return;

    Edge * edge = *it;
    if (!edge)
        return;

    glm::vec3 norm;

    *u = vertex(edge->nextVertex())->point() - vertex(edge->prevVertex())->point();
    it++;
    edge = *it;
    *v = vertex(edge->nextVertex())->point() - vertex(edge->prevVertex())->point();

    norm = glm::normalize(glm::cross(*u, *v));
    *v = glm::normalize(glm::cross(*u, norm));
    *u = glm::normalize(glm::cross(*v, norm));
}

Index WingedEdgeBRep::addVertex(const glm::vec3 & v) {
    Index index = 0;
    for (; index < static_cast<Index>(vertices_.size()); index++) {
        if (vertices_[index] == v)
            return index;
    }
    Vertex vert(v);
    vertices_.push_back(vert);
    return static_cast<Index>(vertices_.size() - 1);
}

Index WingedEdgeBRep::addEdge(Index leftVertex, Index rightVertex) {
    Edge e(leftVertex, rightVertex);
    Index index = 0;
    for (; index < static_cast<Index>(edges_.size()); index++) {
    // if ((edges_[index].prevVertex() == leftVertex) && (edges_[index].nextVertex() == rightVertex))
        if (edges_[index] == e) {
            return index;
        }
    }
    edges_.push_back(e);
    return static_cast<Index>(edges_.size() - 1);
}

void WingedEdgeBRep::addFace(const std::vector<glm::vec3> & vertices, const glm::vec3 & normal, bool winding) {
    // add vertices
    std::vector<Index> indices;
    std::vector<glm::vec3>::const_iterator it = vertices.begin();
    for (; it != vertices.end(); it++) {
        indices.push_back(addVertex(*it));
    }
    // add edges, each from one vertex to the next and the last one back to the first
    std::vector<Index> edges;
    for (Index index = 0; index < static_cast<Index>(indices.size()); index++) {
        const Index leftVertex = indices[index];
        const Index rightVertex = (index == indices.size() - 1) ? indices[0] : indices[index + 1];
        edges.push_back(addEdge(leftVertex, rightVertex));
    }

    // add face
    Face f(normal, edges[0]);
    faces_.push_back(f);
    const Index faceID = static_cast<Index>(faces_.size() - 1);

    // finish constructing edges. The two windings name the same two neighbours through
    // their own half of the record: ccw fills the next side and cw the prev one.
    for (Index index = 0; index < static_cast<Index>(edges.size()); index++) {
        Edge * ep = edge(edges[index]);
        assert(ep != 0);
        const Index following = (index == edges.size() - 1) ? edges[0] : edges[index + 1];
        const Index preceding = (index == 0) ? edges[edges.size() - 1] : edges[index - 1];
        if (winding) {
            ep->nextFace(faceID);
            ep->nextCCWEdge(following);
            ep->nextCWEdge(preceding);
        } else {
            ep->prevFace(faceID);
            ep->prevCWEdge(following);
            ep->prevCCWEdge(preceding);
        }
    }
}

Edge * WingedEdgeBRep::edge(Index edgeID) {
    if (edgeID < edges_.size())
        return &edges_[edgeID];
    return 0;
}

Face * WingedEdgeBRep::face(Index faceID) {
    if (faceID < faces_.size())
        return &faces_[faceID];
    return 0;
}

Vertex * WingedEdgeBRep::vertex(Index vertexID) {
    if (vertexID < vertices_.size())
        return &vertices_[vertexID];
    return 0;
}

size_t WingedEdgeBRep::vertexCount(void) const {
    return vertices_.size();
}

size_t WingedEdgeBRep::edgeCount(void) const {
    return edges_.size();
}

size_t WingedEdgeBRep::faceCount(void) const {
    return faces_.size();
}

Index WingedEdgeBRep::addVertex(const Vertex & v) {
    vertices_.push_back(v);
    return static_cast<Index>(vertices_.size() - 1);
}

Index WingedEdgeBRep::addEdge(const Edge & e) {
    edges_.push_back(e);
    return static_cast<Index>(edges_.size() - 1);
}

Index WingedEdgeBRep::addFace(const Face & f) {
    faces_.push_back(f);
    return static_cast<Index>(faces_.size() - 1);
}

void WingedEdgeBRep::splitEdge(Index edgeID, const glm::vec3 & point) {
    Index vertexID;
    vertices_.push_back(point);
    vertexID = static_cast<Index>(vertices_.size() - 1);

    Edge newEdge = edges_[edgeID];
    // edge goes from PVT to point
    // newEdge goes from point to NVT
    edges_[edgeID].nextVertex(vertexID);
    newEdge.prevVertex(vertexID);

    Index newEdgeID;
    edges_.push_back(newEdge);
    newEdgeID = static_cast<Index>(edges_.size() - 1);

    edges_[edgeID].nextCCWEdge(newEdgeID);
    edges_[edgeID].prevCWEdge(newEdgeID);

    edges_[newEdgeID].nextCWEdge(edgeID);
    edges_[newEdgeID].prevCCWEdge(edgeID);
}

// calculate object-space bounds of mesh
v3d::type::AABBox WingedEdgeBRep::bound(void) const {
    v3d::type::AABBox extents;
    if (vertices_.empty())
        return extents;
    glm::vec3 min;
    glm::vec3 max;
    min = vertices_[0].point();
    max = min;
    glm::vec3 vt;
    for (Index index = 1; index < static_cast<Index>(vertices_.size()); index++) {
        vt = vertices_[index].point();
        min = glm::min(min, vt);
        max = glm::max(max, vt);
    }
    extents.extents(min, max);
    return extents;
}

bool WingedEdgeBRep::selected(void) const noexcept {
    return selected_;
}

void WingedEdgeBRep::selected(bool sel) noexcept {
    selected_ = sel;
    if (!selected_) {
        // make sure components are unselected also
        Index vertex_count = 0;
        for (; vertex_count < vertices_.size(); vertex_count++) {
            vertices_[vertex_count].selected(false);
        }
        Index face_count = 0;
        for (; face_count < faces_.size(); face_count++) {
            faces_[face_count].selected(false);
        }
        Index edge_count = 0;
        for (; edge_count < edges_.size(); edge_count++) {
            edges_[edge_count].selected(false);
        }
    }
}

glm::vec3 WingedEdgeBRep::translation(void) const {
    return v3d::dag::Transform::translation();
}

void WingedEdgeBRep::translation(const glm::vec3 & t) {
    bool component_translate = false;

    // translate selected vertices
    Index vertex_count = 0;
    for (; vertex_count < vertices_.size(); vertex_count++) {
        if (vertices_[vertex_count].selected()) {
            component_translate = true;
            vertices_[vertex_count].point(vertices_[vertex_count].point() + t);
        }
    }

    // translate selected faces
    Index face_count = 0;
    for (; face_count < faces_.size(); face_count++) {
        if (faces_[face_count].selected()) {
            component_translate = true;
            vertex_iterator it(this, face_count);
            for (; *it != 0; it++) {
                Vertex * vert = *it;
                vert->point(vert->point() + t);
            }
        }
    }

    // translate selected edges
    Index edge_count = 0;
    for (; edge_count < edges_.size(); edge_count++) {
        if (edges_[edge_count].selected()) {
            component_translate = true;
            vertices_[edges_[edge_count].prevVertex()].point(vertices_[edges_[edge_count].prevVertex()].point() + t);
            vertices_[edges_[edge_count].nextVertex()].point(vertices_[edges_[edge_count].nextVertex()].point() + t);
        }
    }

    // translate object
    if (!component_translate)
        v3d::dag::Transform::translation(t);
}

};  // namespace v3d::brep
