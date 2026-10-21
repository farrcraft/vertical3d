/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Topology.h"

#include "BRep.h"
#include "Face.h"
#include "HalfEdge.h"
#include "Index.h"
#include "Vertex.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include <glm/geometric.hpp>

namespace v3d::brep {

std::vector<Index> faceLoop(const BRep & mesh, Index face) {
    std::vector<Index> edges;
    const Face* start = mesh.face(face);
    if (start == nullptr) {
        return edges;
    }
    // an edge is visited at most once, so a chain that never returns to the start ends
    std::vector<bool> seen(mesh.edgeCount(), false);
    Index current = start->edge();
    const HalfEdge* edge = mesh.edge(current);
    while (edge != nullptr && !seen[current]) {
        seen[current] = true;
        edges.push_back(current);
        const Index next = edge->next();
        if (next == start->edge()) {
            break;
        }
        current = next;
        // null when next is INVALID_ID or past the end of the mesh
        edge = mesh.edge(current);
    }
    return edges;
}

std::optional<Segment> loopSegment(const BRep & mesh, const std::vector<Index> & loop, std::size_t entry) {
    if (loop.empty() || entry >= loop.size()) {
        return std::nullopt;
    }
    const HalfEdge* edge = mesh.edge(loop[entry]);
    const HalfEdge* before = mesh.edge(loop[(entry + loop.size() - 1) % loop.size()]);
    if (edge == nullptr || before == nullptr) {
        return std::nullopt;
    }
    const Vertex* start = mesh.vertex(before->vertex());
    const Vertex* end = mesh.vertex(edge->vertex());
    if (start == nullptr || end == nullptr) {
        return std::nullopt;
    }
    return Segment{.from = start->point(), .to = end->point()};
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
    glm::vec3 sum(0.0F);
    float count = 0.0F;
    for (const Index entry : loop) {
        const Vertex* vertex = mesh.vertex(mesh.edge(entry)->vertex());
        if (vertex != nullptr) {
            sum += vertex->point();
            count += 1.0F;
        }
    }
    return count > 0.0F ? sum / count : sum;
}

void faceUV(const BRep & mesh, Index face, glm::vec3* u, glm::vec3* v) {
    const std::vector<Index> loop = faceLoop(mesh, face);
    if (loop.size() < 2) {
        return;
    }
    // each edge's direction is from the vertex the next edge ends at back to its own
    std::vector<glm::vec3> sides;
    sides.reserve(loop.size());
    for (std::size_t entry = 0; entry < loop.size(); entry++) {
        const Vertex* end = mesh.vertex(mesh.edge(loop[entry])->vertex());
        const Vertex* after = mesh.vertex(mesh.edge(loop[(entry + 1) % loop.size()])->vertex());
        if (end == nullptr || after == nullptr) {
            return;
        }
        sides.push_back(end->point() - after->point());
    }
    // the first side that has a length, and the first side after it that is not parallel to it
    constexpr float PARALLEL = 1e-10F;
    for (std::size_t first = 0; first < sides.size(); first++) {
        const float firstLength = glm::dot(sides[first], sides[first]);
        if (firstLength <= 0.0F) {
            continue;
        }
        for (std::size_t second = first + 1; second < sides.size(); second++) {
            const glm::vec3 normal = glm::cross(sides[first], sides[second]);
            const float secondLength = glm::dot(sides[second], sides[second]);
            if (glm::dot(normal, normal) > PARALLEL * firstLength * secondLength) {
                const glm::vec3 unitNormal = glm::normalize(normal);
                *v = glm::normalize(glm::cross(sides[first], unitNormal));
                *u = glm::normalize(glm::cross(*v, unitNormal));
                return;
            }
        }
        return;
    }
}

std::vector<Index> edgesFrom(const BRep & mesh, Index vertex) {
    std::vector<Index> leaving;
    const std::size_t count = mesh.edgeCount();
    for (Index id = 0; id < count; id++) {
        const HalfEdge* arriving = mesh.edge(id);
        if (arriving == nullptr || arriving->vertex() != vertex || mesh.edge(arriving->next()) == nullptr) {
            continue;
        }
        leaving.push_back(arriving->next());
    }
    std::sort(leaving.begin(), leaving.end());
    leaving.erase(std::unique(leaving.begin(), leaving.end()), leaving.end());
    return leaving;
}

std::vector<Index> facesAround(const BRep & mesh, Index vertex) {
    std::vector<Index> faces;
    const std::size_t count = mesh.edgeCount();
    for (Index id = 0; id < count; id++) {
        const HalfEdge* arriving = mesh.edge(id);
        if (arriving == nullptr || arriving->vertex() != vertex || mesh.face(arriving->face()) == nullptr) {
            continue;
        }
        faces.push_back(arriving->face());
    }
    std::sort(faces.begin(), faces.end());
    faces.erase(std::unique(faces.begin(), faces.end()), faces.end());
    return faces;
}

glm::vec3 newellNormal(const BRep & mesh, Index face) {
    std::vector<glm::vec3> points;
    for (const Index id : faceLoop(mesh, face)) {
        const Vertex* vertex = mesh.vertex(mesh.edge(id)->vertex());
        if (vertex != nullptr) {
            points.push_back(vertex->point());
        }
    }
    glm::vec3 normal(0.0F);
    if (points.size() < 3) {
        return normal;
    }
    // each edge adds the area it sweeps on the three axis planes. The sum points to the side
    // the loop runs counter-clockwise when seen from, which is inside
    for (std::size_t index = 0; index < points.size(); index++) {
        const glm::vec3& current = points[index];
        const glm::vec3& next = points[(index + 1) % points.size()];
        normal.x += (current.y - next.y) * (current.z + next.z);
        normal.y += (current.z - next.z) * (current.x + next.x);
        normal.z += (current.x - next.x) * (current.y + next.y);
    }
    const float length = glm::length(normal);
    if (length <= 0.0F) {
        return glm::vec3(0.0F);
    }
    return -normal / length;
}

};  // namespace v3d::brep
