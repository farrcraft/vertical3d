/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "BRep.h"

#include <algorithm>
#include <cassert>
#include <vector>


namespace v3d::brep {
    const Index BRep::INVALID_ID = v3d::brep::INVALID_ID;


    BRep::edge_iterator::edge_iterator() : firstEdge_(INVALID_ID), edge_(0) {
    }

    BRep::edge_iterator::edge_iterator(boost::shared_ptr<BRep> brep, Index face) {
        reset(brep, face);
    }

    BRep::edge_iterator::~edge_iterator() {
    }

    void BRep::edge_iterator::reset(boost::shared_ptr<BRep> brep, Index face_id) {
        brep_ = brep;
        Face* face = brep_->face(face_id);
        if (!face) {
            edge_ = 0;
            return;
        }
        firstEdge_ = face->edge();
        edge_ = brep_->edge(firstEdge_);
    }

    HalfEdge* BRep::edge_iterator::operator * () {
        return edge_;
    }

    BRep::edge_iterator BRep::edge_iterator::operator++ (int) {
        if (!edge_)
            return *this;
        Index nextEdge = edge_->next();
        if (nextEdge == firstEdge_) {
            edge_ = 0;
            return *this;
        }
        edge_ = brep_->edge(nextEdge);

        return *this;
    }

    boost::shared_ptr<BRep> BRep::edge_iterator::brep(void) const {
        return brep_;
    }

    BRep::vertex_iterator::vertex_iterator() {
    }

    BRep::vertex_iterator::vertex_iterator(boost::shared_ptr<BRep> brep, Index faceID) {
        reset(brep, faceID);
    }

    BRep::vertex_iterator::~vertex_iterator() {
    }

    Vertex* BRep::vertex_iterator::operator * () {
        if ((*iterator_) == 0)
            return 0;
        return (iterator_.brep()->vertex((*iterator_)->vertex()));
    }

    BRep::vertex_iterator BRep::vertex_iterator::operator++ (int) {
        if ((*iterator_) == 0)
            return *this;
        iterator_++;
        return *this;
    }

    void BRep::vertex_iterator::reset(boost::shared_ptr<BRep> brep, Index face) {
        iterator_.reset(brep, face);
    }


    BRep::BRep() : selected_(false) {
    }

    BRep::~BRep() {
    }

    glm::vec3 center(boost::shared_ptr<BRep> mesh, Index face) {
        float nverts = 0.0;
        glm::vec3 mid(0.0, 0.0, 0.0);

        BRep::vertex_iterator it(mesh, face);
        Vertex* vert;
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

    void faceUV(boost::shared_ptr<BRep> mesh, Index face, glm::vec3* u, glm::vec3* v) {
        BRep::edge_iterator it(mesh, face);
        if (*it == 0)
            return;

        HalfEdge* edge = *it;
        if (!edge)
            return;

        glm::vec3 norm;

        Index vert = edge->vertex();
        assert(vert != BRep::INVALID_ID);
        Index pair = edge->next();  // edge->pair();
        assert(pair != BRep::INVALID_ID);
        Index pair_vert = mesh->edge(pair)->vertex();
        assert(pair_vert != BRep::INVALID_ID);
        *u = mesh->vertex(vert)->point() - mesh->vertex(pair_vert)->point();
        it++;
        edge = *it;
        assert(edge != 0);
        vert = edge->vertex();
        assert(vert != BRep::INVALID_ID);
        pair = edge->next();  // edge->pair();
        assert(pair != BRep::INVALID_ID);
        pair_vert = mesh->edge(pair)->vertex();
        assert(pair_vert != BRep::INVALID_ID);
        *v = mesh->vertex(vert)->point() - mesh->vertex(pair_vert)->point();

        norm = glm::normalize(glm::cross(*u, *v));
        *v = glm::normalize(glm::cross(*u, norm));
        *u = glm::normalize(glm::cross(*v, norm));
    }

    Index BRep::addVertex(const glm::vec3& v) {
        Index index = 0;
        for (; index < static_cast<Index>(vertices_.size()); index++) {
            if (vertices_[index] == v)
                return index;
        }
        Vertex vert(v);
        vertices_.push_back(vert);
        return static_cast<Index>(vertices_.size() - 1);
    }

    Index BRep::addEdge(Index vertex) {
        HalfEdge e(vertex);
        /*
        Index index = 0;
        for (; index < _edges.size(); index++)
        {
            if (_edges[index] == e)
            {
                return index;
            }
        }
        */
        edges_.push_back(e);
        return static_cast<Index>(edges_.size() - 1);
    }

    void BRep::addFace(const std::vector<glm::vec3>& vertices, const glm::vec3& normal) {
        // add vertices
        std::vector<Index> indices;
        Index vertex;
        std::vector<glm::vec3>::const_iterator it = vertices.begin();
        for (; it != vertices.end(); it++) {
            vertex = addVertex(*it);
            indices.push_back(vertex);
        }

        // add edges
        Index index = 0;
        Index edge_id;
        /*
        Index firstEdgeID = INVALID_ID;
        Index lastEdgeID = INVALID_ID;
        */
        std::vector<Index> edges;
        for (; index < static_cast<Index>(indices.size()); index++) {
            edge_id = addEdge(indices[index]);
            /*
            if (index != 0)
                _edges[edgeID].pair(indices[index - 1]);
            if (lastEdgeID != INVALID_ID)
                _edges[lastEdgeID].next(edgeID);

            if (index == (indices.size() - 1))
            {
                _edges[firstEdgeID].pair(edgeID);
                _edges[edgeID].next(firstEdgeID);
            }

            if (firstEdgeID == INVALID_ID)
                firstEdgeID = edgeID;
            lastEdgeID = edgeID;
            */
            edges.push_back(edge_id);
        }

        // add face
        Index face;
        Face f(normal, edges[0]);
        faces_.push_back(f);
        face = static_cast<Index>(faces_.size() - 1);

        // finish constructing edges
        HalfEdge* ep;

        for (index = 0; index < static_cast<Index>(edges.size()); index++) {
            ep = edge(edges[index]);
            assert(ep != 0);
            ep->face(face);
            Index next;
            if (index != static_cast<Index>(edges.size()) - 1)
                next = edges[index + 1];
            else
                next = edges[0];
            ep->next(next);
        }

        for (index = 0; index < static_cast<Index>(edges.size()); index++) {
            ep = edge(edges[index]);
            assert(ep != 0);
            Index pair = INVALID_ID;
            if (index != 0)
                pair = findPair(edges[index], edges[index - 1]);
            else
                pair = findPair(edges[index], edges[static_cast<Index>(edges.size()) - 1]);
            //  assert(pair != INVALID_ID);
            if (pair != INVALID_ID) {
                ep->pair(pair);
                edge(pair)->pair(edges[index]);
            }
        }
    }

    Index BRep::findPair(Index edge, Index prevEdge) {
        // this edge's pair points to the previous edge/vertex in this face
        // find the edge that points to the previous edge's vertex
        // and whose next edge points to this edge's vertex

        Index index = 0;
        // pairs never share the same face
        const Index face = edges_[edge].face();
        for (; index < static_cast<Index>(edges_.size()); index++) {
            if (edges_[index].face() != face &&
                edges_[index].vertex() == edges_[prevEdge].vertex()) {
                // find potential pair's prev edge
                Index pair_prev = edges_[index].next();
                assert(pair_prev != INVALID_ID);
                for (; edges_[pair_prev].next() != index; ) {
                    pair_prev = edges_[pair_prev].next();
                    assert(pair_prev != INVALID_ID);
                }
                if (edges_[pair_prev].vertex() == edges_[edge].vertex())
                    return index;
            }
            /* #2
                if (_edges[index].face() != face &&
                    _edges[index].vertex() == _edges[edgeID].vertex())
                {
                    Index next = _edges[index].next();
                    assert(next != INVALID_ID);
                    if (_edges[next].vertex() == _edges[prevEdgeID].vertex())
                        return index;
                }
            */
            /* #1
            if (index != edgeID && index != prevEdgeID &&
                _edges[index].vertex() == _edges[prevEdgeID].vertex())
            {
                if (_edges[index].next() != INVALID_ID)
                {
                    if (_edges[_edges[index].next()].vertex() == _edges[edgeID].vertex())
                        return index;
                }
            }
            */
        }
        return INVALID_ID;
    }

    HalfEdge* BRep::edge(Index edge) {
        if (edge < edges_.size())
            return &edges_[edge];
        return 0;
    }

    Face* BRep::face(Index face) {
        if (face < faces_.size())
            return &faces_[face];
        return 0;
    }

    Vertex* BRep::vertex(Index vert) {
        if (vert < vertices_.size())
            return &vertices_[vert];
        return 0;
    }

    size_t BRep::vertexCount(void) const {
        return vertices_.size();
    }

    size_t BRep::edgeCount(void) const {
        return edges_.size();
    }

    size_t BRep::faceCount(void) const {
        return faces_.size();
    }

    Index BRep::addVertex(const Vertex& v) {
        vertices_.push_back(v);
        return static_cast<Index>(vertices_.size() - 1);
    }

    Index BRep::addEdge(const HalfEdge& e) {
        edges_.push_back(e);
        return static_cast<Index>(edges_.size() - 1);
    }

    Index BRep::addFace(const Face& f) {
        faces_.push_back(f);
        return static_cast<Index>(faces_.size() - 1);
    }

    void BRep::splitEdge(Index edge, const glm::vec3& point) {
        Index vertex;
        vertices_.push_back(point);
        vertex = static_cast<Index>(vertices_.size() - 1);

        HalfEdge newEdge(edges_[edge]);
        // edge goes from PVT to point
        // newEdge goes from point to NVT
        newEdge.vertex(vertex);

        Index new_edge_id;
        edges_.push_back(newEdge);
        new_edge_id = static_cast<Index>(edges_.size() - 1);

        newEdge.pair(edge);
        newEdge.next(edges_[edge].next());
        edges_[edge].next(new_edge_id);
    }

    bool BRep::selected(void) const noexcept {
        return selected_;
    }

    void BRep::selected(bool sel) noexcept {
        selected_ = sel;
    }

    void BRep::deselectComponents(void) noexcept {
        for (Vertex& vert : vertices_) {
            vert.selected(false);
        }
        for (HalfEdge& e : edges_) {
            e.selected(false);
        }
        for (Face& f : faces_) {
            f.selected(false);
        }
    }

    // calculate object-space bounds of mesh
    v3d::type::AABBox BRep::bound(void) const {
        v3d::type::AABBox extents;
        if (vertices_.size() == 0)
            return extents;
        glm::vec3 min, max;
        min = vertices_[0].point();
        max = min;
        glm::vec3 vt;
        for (Index index = 1; index < static_cast<Index>(vertices_.size()); index++) {
            vt = vertices_[index].point();
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

};  // namespace v3d::brep
