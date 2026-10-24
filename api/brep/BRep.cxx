/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "BRep.h"

#include <api/type/geometry/AABBox.h>

#include "Face.h"
#include "HalfEdge.h"
#include "Index.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <glm/common.hpp>

namespace v3d::brep {
const Index BRep::INVALID_ID = v3d::brep::INVALID_ID;


BRep::BRep() : selected_(false) {
}

BRep::~BRep() {
}

Index BRep::addVertex(const glm::vec3& v) {
    Index index = 0;
    for (; index < static_cast<Index>(vertices_.size()); index++) {
        if (vertices_[index] == v) {
            return index;
        }
    }
    const Vertex vert(v);
    vertices_.push_back(vert);
    return static_cast<Index>(vertices_.size() - 1);
}

Index BRep::addEdge(Index vertex) {
    const HalfEdge halfEdge(vertex);
    edges_.push_back(halfEdge);
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
    std::vector<Index> edges;
    for (; index < static_cast<Index>(indices.size()); index++) {
        edge_id = addEdge(indices[index]);
        edges.push_back(edge_id);
    }

    // add face
    Index face;
    const Face newFace(normal, edges[0]);
    faces_.push_back(newFace);
    face = static_cast<Index>(faces_.size() - 1);

    // finish constructing edges
    HalfEdge* ep;

    for (index = 0; index < static_cast<Index>(edges.size()); index++) {
        ep = edge(edges[index]);
        assert(ep != 0);
        ep->face(face);
        Index next;
        if (index != static_cast<Index>(edges.size()) - 1) {
            next = edges[index + 1];
        } else {
            next = edges[0];
        }
        ep->next(next);
    }

    for (index = 0; index < static_cast<Index>(edges.size()); index++) {
        ep = edge(edges[index]);
        assert(ep != 0);
        Index pair = INVALID_ID;
        if (index != 0) {
            pair = findPair(edges[index], edges[index - 1]);
        } else {
            pair = findPair(edges[index], edges[static_cast<Index>(edges.size()) - 1]);
        }
        if (pair != INVALID_ID) {
            ep->pair(pair);
            edge(pair)->pair(edges[index]);
        }
    }
}

Index BRep::findPair(Index edge, Index prevEdge) {
    // a half edge runs from the vertex prevEdge ends at to the vertex it ends at itself. Its
    // pair runs the other way, in another face. The pair therefore ends at prevEdge's vertex,
    // and the half edge before the pair in its loop ends at this edge's vertex

    Index index = 0;
    // pairs never share the same face
    const Index face = edges_[edge].face();
    for (; index < static_cast<Index>(edges_.size()); index++) {
        if (edges_[index].face() != face &&
            edges_[index].vertex() == edges_[prevEdge].vertex()) {
            // find the half edge before the candidate in its loop. The walk is bounded by the
            // edge count, so a ring that does not close ends the search
            Index pair_prev = edges_[index].next();
            std::size_t steps = 0;
            while (pair_prev < edges_.size() && edges_[pair_prev].next() != index && steps++ < edges_.size()) {
                pair_prev = edges_[pair_prev].next();
            }
            if (pair_prev < edges_.size() && edges_[pair_prev].next() == index &&
                edges_[pair_prev].vertex() == edges_[edge].vertex()) {
                return index;
            }
        }
    }
    return INVALID_ID;
}

HalfEdge* BRep::edge(Index index) {
    if (index < edges_.size()) {
        return &edges_[index];
    }
    return 0;
}

Face* BRep::face(Index index) {
    if (index < faces_.size()) {
        return &faces_[index];
    }
    return 0;
}

Vertex* BRep::vertex(Index vert) {
    if (vert < vertices_.size()) {
        return &vertices_[vert];
    }
    return 0;
}

const HalfEdge* BRep::edge(Index index) const {
    return index < edges_.size() ? &edges_[index] : nullptr;
}

const Face* BRep::face(Index index) const {
    return index < faces_.size() ? &faces_[index] : nullptr;
}

const Vertex* BRep::vertex(Index vert) const {
    return vert < vertices_.size() ? &vertices_[vert] : nullptr;
}

namespace {

/**
 * Whether a reference is to something there are this many of, or is the sentinel where one
 * may be absent.
 **/
bool refers(Index reference, std::size_t count, bool optional) {
    if (reference == INVALID_ID) {
        return optional;
    }
    return reference < count;
}

/**
 * Report a problem, for a check that has found one.
 * @return false, so a check can return what this returns
 **/
bool fail(std::string* problem, const std::string& what) {
    if (problem != nullptr) {
        *problem = what;
    }
    return false;
}

/**
 * Whether every half edge that has a pair is that pair's pair, and not its own.
 **/
bool pairsMutual(const BRep& mesh, std::string* problem) {
    for (Index id = 0; id < mesh.edgeCount(); id++) {
        const Index pair = mesh.edge(id)->pair();
        if (pair != INVALID_ID && (pair == id || mesh.edge(pair)->pair() != id)) {
            return fail(problem, "edge " + std::to_string(id) + "'s pair " + std::to_string(pair) + " does not pair it back");
        }
    }
    return true;
}

/**
 * Whether a face's next chain closes, every half edge on it names the face, and no two of
 * them run between the same two vertices. The mesh has passed validate(), so every index it
 * holds is in range or INVALID_ID.
 **/
bool loopSound(const BRep& mesh, Index face, std::string* problem) {
    const std::string name = "face " + std::to_string(face);
    const Index start = mesh.face(face)->edge();
    std::vector<Index> loop;
    std::vector<bool> seen(mesh.edgeCount(), false);
    Index current = start;
    do {
        if (current == INVALID_ID || seen[current]) {
            return fail(problem, name + "'s loop does not close");
        }
        seen[current] = true;
        loop.push_back(current);
        const Index named = mesh.edge(current)->face();
        if (named != face) {
            return fail(problem, "edge " + std::to_string(current) + " is in " + name + "'s loop and names face " +
                (named == INVALID_ID ? std::string("none") : std::to_string(named)));
        }
        current = mesh.edge(current)->next();
    } while (current != start);

    // a half edge runs from the vertex the one before it ends at to the vertex it ends at
    std::set<std::pair<Index, Index>> spans;
    for (std::size_t entry = 0; entry < loop.size(); entry++) {
        const Index from = mesh.edge(loop[(entry + loop.size() - 1) % loop.size()])->vertex();
        const Index to = mesh.edge(loop[entry])->vertex();
        if (!spans.insert(std::minmax(from, to)).second) {
            return fail(problem, name + " has two edges between vertices " + std::to_string(std::min(from, to)) +
                " and " + std::to_string(std::max(from, to)));
        }
    }
    return true;
}

};  // namespace

bool BRep::validate(std::string* problem) const {
    for (std::size_t id = 0; id < edges_.size(); id++) {
        const HalfEdge & edge = edges_[id];
        if (!refers(edge.vertex(), vertices_.size(), false) || !refers(edge.face(), faces_.size(), true) ||
            !refers(edge.pair(), edges_.size(), true) || !refers(edge.next(), edges_.size(), true)) {
            if (problem != nullptr) {
                *problem = "edge " + std::to_string(id) + " names something the mesh does not hold";
            }
            return false;
        }
    }
    for (std::size_t id = 0; id < faces_.size(); id++) {
        if (!refers(faces_[id].edge(), edges_.size(), false)) {
            if (problem != nullptr) {
                *problem = "face " + std::to_string(id) + " names an edge the mesh does not hold";
            }
            return false;
        }
    }
    return true;
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

Index BRep::addEdge(const HalfEdge& newEdge) {
    edges_.push_back(newEdge);
    return static_cast<Index>(edges_.size() - 1);
}

Index BRep::addFace(const Face& newFace) {
    faces_.push_back(newFace);
    return static_cast<Index>(faces_.size() - 1);
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
    for (HalfEdge& halfEdge : edges_) {
        halfEdge.selected(false);
    }
    for (Face& polygon : faces_) {
        polygon.selected(false);
    }
}

BRep::Snapshot BRep::snapshot(void) const {
    Snapshot snapshot;
    snapshot.vertices = vertices_;
    snapshot.edges = edges_;
    snapshot.faces = faces_;
    return snapshot;
}

void BRep::restore(const Snapshot& snapshot) {
    vertices_ = snapshot.vertices;
    edges_ = snapshot.edges;
    faces_ = snapshot.faces;
}

bool BRep::validateStrict(std::string* problem) const {
    if (!validate(problem)) {
        return false;
    }
    if (!pairsMutual(*this, problem)) {
        return false;
    }
    for (Index id = 0; id < faces_.size(); id++) {
        if (!loopSound(*this, id, problem)) {
            return false;
        }
    }
    return true;
}

// calculate object-space bounds of mesh
v3d::type::geometry::AABBox BRep::bound(void) const {
    v3d::type::geometry::AABBox extents;
    if (vertices_.empty()) {
        return extents;
    }
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

};  // namespace v3d::brep
