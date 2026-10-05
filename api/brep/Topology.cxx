/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Topology.h"

#include <cstddef>
#include <vector>

#include <glm/geometric.hpp>

namespace v3d::brep {

std::vector<Index> faceLoop(const BRep & mesh, Index face) {
    std::vector<Index> edges;
    const Face* start = mesh.face(face);
    if (start == nullptr) {
        return edges;
    }
    Index current = start->edge();
    while (edges.size() <= mesh.edgeCount()) {
        const HalfEdge* edge = mesh.edge(current);
        if (edge == nullptr) {
            break;
        }
        edges.push_back(current);
        const Index next = edge->next();
        if (next == INVALID_ID || next == start->edge()) {
            break;
        }
        current = next;
    }
    return edges;
}

bool loopSegment(const BRep & mesh, const std::vector<Index> & loop, std::size_t entry, glm::vec3* from, glm::vec3* to) {
    if (loop.empty() || entry >= loop.size()) {
        return false;
    }
    const HalfEdge* edge = mesh.edge(loop[entry]);
    const HalfEdge* before = mesh.edge(loop[(entry + loop.size() - 1) % loop.size()]);
    if (edge == nullptr || before == nullptr) {
        return false;
    }
    const Vertex* start = mesh.vertex(before->vertex());
    const Vertex* end = mesh.vertex(edge->vertex());
    if (start == nullptr || end == nullptr) {
        return false;
    }
    if (from != nullptr) {
        *from = start->point();
    }
    if (to != nullptr) {
        *to = end->point();
    }
    return true;
}

bool ownsEdge(const BRep & mesh, Index edge) {
    const HalfEdge* half = mesh.edge(edge);
    if (half == nullptr) {
        return false;
    }
    const Index other = half->pair();
    return other == INVALID_ID || other >= edge;
}

bool edgeSelected(const BRep & mesh, Index edge) {
    const HalfEdge* half = mesh.edge(edge);
    if (half == nullptr) {
        return false;
    }
    if (half->selected()) {
        return true;
    }
    const HalfEdge* other = half->pair() == INVALID_ID ? nullptr : mesh.edge(half->pair());
    return other != nullptr && other->selected();
}

glm::vec3 center(const BRep & mesh, Index face) {
    const std::vector<Index> loop = faceLoop(mesh, face);
    glm::vec3 sum(0.0f);
    float count = 0.0f;
    for (const Index entry : loop) {
        const Vertex* vertex = mesh.vertex(mesh.edge(entry)->vertex());
        if (vertex != nullptr) {
            sum += vertex->point();
            count += 1.0f;
        }
    }
    return count > 0.0f ? sum / count : sum;
}

void faceUV(const BRep & mesh, Index face, glm::vec3* u, glm::vec3* v) {
    const std::vector<Index> loop = faceLoop(mesh, face);
    if (loop.size() < 2) {
        return;
    }
    // each edge's direction is from the vertex the next edge ends at back to its own
    glm::vec3 sides[2];
    for (std::size_t side = 0; side < 2; side++) {
        const Vertex* end = mesh.vertex(mesh.edge(loop[side])->vertex());
        const Vertex* after = mesh.vertex(mesh.edge(loop[(side + 1) % loop.size()])->vertex());
        if (end == nullptr || after == nullptr) {
            return;
        }
        sides[side] = end->point() - after->point();
    }
    const glm::vec3 normal = glm::normalize(glm::cross(sides[0], sides[1]));
    *v = glm::normalize(glm::cross(sides[0], normal));
    *u = glm::normalize(glm::cross(*v, normal));
}

};  // namespace v3d::brep
