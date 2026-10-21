/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Operations.h"

#include "BRep.h"
#include "Face.h"
#include "HalfEdge.h"
#include "Index.h"
#include "Topology.h"
#include "Vertex.h"

#include <cstddef>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::brep {

namespace {

/**
 * Cut one half edge in two at a vertex. The half edge now ends at the vertex, and a new half
 * edge carries on from there to where it used to end, in the same face.
 *
 * @return the new half edge
 **/
Index cut(BRep* mesh, Index edge, Index vertex) {
    const HalfEdge& old = *mesh->edge(edge);
    HalfEdge rest(old.vertex());
    rest.face(old.face());
    rest.next(old.next());
    const Index added = mesh->addEdge(rest);
    // the add may have moved the array, so the half edge is looked up again
    mesh->edge(edge)->vertex(vertex);
    mesh->edge(edge)->next(added);
    return added;
}

/**
 * Where in a loop the half edge that ends at a vertex is.
 **/
std::optional<std::size_t> arriving(const BRep& mesh, const std::vector<Index>& loop, Index vertex) {
    for (std::size_t entry = 0; entry < loop.size(); entry++) {
        if (mesh.edge(loop[entry])->vertex() == vertex) {
            return entry;
        }
    }
    return std::nullopt;
}

};  // namespace

std::optional<Index> splitEdge(BRep & mesh, Index edge) {
    if (mesh.edge(edge) == nullptr) {
        return std::nullopt;
    }
    const std::vector<Index> loop = faceLoop(mesh, mesh.edge(edge)->face());
    std::size_t entry = 0;
    while (entry < loop.size() && loop[entry] != edge) {
        entry++;
    }
    const std::optional<Segment> segment = entry < loop.size() ? loopSegment(mesh, loop, entry) : std::nullopt;
    if (!segment) {
        return std::nullopt;
    }

    const Index middle = mesh.addVertex(Vertex((segment->from + segment->to) * 0.5F));
    const Index pair = mesh.edge(edge)->pair();
    const Index beyond = cut(&mesh, edge, middle);
    if (pair == INVALID_ID) {
        return middle;
    }

    // the pair runs the other way, so its first part meets this edge's second part and its
    // second part meets this edge's first
    const Index back = cut(&mesh, pair, middle);
    mesh.edge(edge)->pair(back);
    mesh.edge(back)->pair(edge);
    mesh.edge(beyond)->pair(pair);
    mesh.edge(pair)->pair(beyond);
    return middle;
}

std::optional<Index> splitFace(BRep & mesh, Index face, Index first, Index second) {
    if (mesh.face(face) == nullptr || first == second) {
        return std::nullopt;
    }
    const std::vector<Index> loop = faceLoop(mesh, face);
    const std::optional<std::size_t> intoFirst = arriving(mesh, loop, first);
    const std::optional<std::size_t> intoSecond = arriving(mesh, loop, second);
    if (!intoFirst || !intoSecond) {
        return std::nullopt;
    }
    const std::size_t size = loop.size();
    const Index outOfFirst = loop[(*intoFirst + 1) % size];
    const Index outOfSecond = loop[(*intoSecond + 1) % size];
    if (mesh.edge(outOfFirst)->vertex() == second || mesh.edge(outOfSecond)->vertex() == first) {
        return std::nullopt;
    }

    // the new edge, one half running each way between the two corners
    const Index across = mesh.addEdge(HalfEdge(second));
    const Index returning = mesh.addEdge(HalfEdge(first));
    const Index added = mesh.addFace(Face(glm::vec3(0.0F), returning));
    mesh.edge(across)->pair(returning);
    mesh.edge(returning)->pair(across);

    // the face keeps the corners from the second round to the first, closed by the edge
    // from first to second
    mesh.edge(loop[*intoFirst])->next(across);
    mesh.edge(across)->next(outOfSecond);
    mesh.edge(across)->face(face);
    mesh.face(face)->edge(across);

    // the new face holds the corners from the first round to the second, closed by the edge
    // from second back to first
    mesh.edge(loop[*intoSecond])->next(returning);
    mesh.edge(returning)->next(outOfFirst);
    for (std::size_t step = 1; step <= size; step++) {
        const Index part = loop[(*intoFirst + step) % size];
        mesh.edge(part)->face(added);
        if (part == loop[*intoSecond]) {
            break;
        }
    }
    mesh.edge(returning)->face(added);

    mesh.face(face)->normal(newellNormal(mesh, face));
    mesh.face(added)->normal(newellNormal(mesh, added));
    return added;
}

};  // namespace v3d::brep
