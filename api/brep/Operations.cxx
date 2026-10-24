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
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <glm/common.hpp>
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

/**
 * Group faces into regions. Two faces are in one region when they share an edge, directly or
 * through other faces of the set.
 *
 * @return the region of each face of the set, by face index
 **/
std::map<Index, std::size_t> regions(const BRep& mesh, const std::vector<Index>& faces) {
    const std::set<Index> chosen(faces.begin(), faces.end());
    std::map<Index, std::size_t> region;
    std::size_t count = 0;
    for (const Index start : chosen) {
        if (region.contains(start)) {
            continue;
        }
        std::vector<Index> open{start};
        region[start] = count;
        while (!open.empty()) {
            const Index face = open.back();
            open.pop_back();
            for (const Index edge : faceLoop(mesh, face)) {
                const Index pair = mesh.edge(edge)->pair();
                if (pair == INVALID_ID || mesh.edge(pair) == nullptr) {
                    continue;
                }
                const Index beside = mesh.edge(pair)->face();
                if (chosen.contains(beside) && !region.contains(beside)) {
                    region[beside] = count;
                    open.push_back(beside);
                }
            }
        }
        count++;
    }
    return region;
}

/**
 * One half edge of a region's outline: a half edge of a face in the region whose other side is
 * not in the region.
 **/
struct Rim {
    Index edge = INVALID_ID;
    Index from = INVALID_ID;
    Index to = INVALID_ID;
    // the half edge on the other side, or INVALID_ID on the mesh's boundary
    Index outside = INVALID_ID;
    // the half edge of the outline that leaves where this one ends
    Index after = INVALID_ID;
    std::size_t region = 0;
};

/**
 * The half edge of a region's outline that leaves the vertex an outline half edge ends at. It is
 * found by turning about the vertex through the region's faces until the next edge crosses out
 * of the region.
 **/
Index outlineAfter(const BRep& mesh, const std::map<Index, std::size_t>& region, Index edge) {
    Index leaving = mesh.edge(edge)->next();
    for (std::size_t turn = 0; turn < mesh.edgeCount() && leaving != INVALID_ID; turn++) {
        const Index back = mesh.edge(leaving)->pair();
        if (back == INVALID_ID || !region.contains(mesh.edge(back)->face())) {
            return leaving;
        }
        leaving = mesh.edge(back)->next();
    }
    return INVALID_ID;
}

/**
 * The outline of every region, in the order of the faces and then of their loops.
 **/
std::vector<Rim> outline(const BRep& mesh, const std::map<Index, std::size_t>& region) {
    std::vector<Rim> rims;
    for (const auto& [face, group] : region) {
        const std::vector<Index> loop = faceLoop(mesh, face);
        for (std::size_t entry = 0; entry < loop.size(); entry++) {
            const Index outside = mesh.edge(loop[entry])->pair();
            if (outside != INVALID_ID && region.contains(mesh.edge(outside)->face())) {
                continue;
            }
            Rim rim;
            rim.edge = loop[entry];
            rim.from = mesh.edge(loop[(entry + loop.size() - 1) % loop.size()])->vertex();
            rim.to = mesh.edge(loop[entry])->vertex();
            rim.outside = outside;
            rim.after = outlineAfter(mesh, region, loop[entry]);
            rim.region = group;
            rims.push_back(rim);
        }
    }
    return rims;
}

/** The copy of each corner on a region's outline, by region and corner. **/
using Copies = std::map<std::pair<std::size_t, Index>, Index>;

/**
 * Copy each corner on a region's outline once for that region, and move the region's faces onto
 * the copies.
 **/
Copies copyCorners(BRep* mesh, const std::map<Index, std::size_t>& region, const std::vector<Rim>& rims) {
    Copies copies;
    for (const Rim& rim : rims) {
        for (const Index corner : {rim.from, rim.to}) {
            if (!copies.contains({rim.region, corner})) {
                copies[{rim.region, corner}] = mesh->addVertex(Vertex(mesh->vertex(corner)->point()));
            }
        }
    }
    for (const auto& [face, group] : region) {
        for (const Index edge : faceLoop(*mesh, face)) {
            const auto copy = copies.find({group, mesh->edge(edge)->vertex()});
            if (copy != copies.end()) {
                mesh->edge(edge)->vertex(copy->second);
            }
        }
    }
    return copies;
}

/** One quad of a ring, and its two half edges that run between a corner and its copy. **/
struct Quad {
    Index face = INVALID_ID;
    Index falling = INVALID_ID;
    Index rising = INVALID_ID;
};

/**
 * The quad of a ring for one outline half edge, from a to b: b' to a' beside the region's half
 * edge, a' down to a, a to b beside the half edge outside, and b up to b'. It is paired with the
 * two half edges it lies beside, and not yet with the quads beside it.
 **/
Quad ringQuad(BRep* mesh, const Rim& rim, const Copies& copies) {
    const Index along = mesh->addEdge(HalfEdge(copies.at({rim.region, rim.from})));
    const Index falling = mesh->addEdge(HalfEdge(rim.from));
    const Index below = mesh->addEdge(HalfEdge(rim.to));
    const Index rising = mesh->addEdge(HalfEdge(copies.at({rim.region, rim.to})));
    const Index face = mesh->addFace(Face(glm::vec3(0.0F), along));
    const std::vector<Index> sides{along, falling, below, rising};
    for (std::size_t side = 0; side < sides.size(); side++) {
        mesh->edge(sides[side])->next(sides[(side + 1) % sides.size()]);
        mesh->edge(sides[side])->face(face);
    }
    mesh->edge(along)->pair(rim.edge);
    mesh->edge(rim.edge)->pair(along);
    if (rim.outside != INVALID_ID) {
        mesh->edge(below)->pair(rim.outside);
        mesh->edge(rim.outside)->pair(below);
    }
    return Quad{.face = face, .falling = falling, .rising = rising};
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

std::optional<std::vector<Index>> extrudeFaces(BRep & mesh, const std::vector<Index> & faces) {
    for (const Index face : faces) {
        if (mesh.face(face) == nullptr) {
            return std::nullopt;
        }
    }
    const std::map<Index, std::size_t> region = regions(mesh, faces);
    const std::vector<Rim> rims = outline(mesh, region);
    const Copies copies = copyCorners(&mesh, region, rims);

    std::vector<Quad> quads;
    std::map<Index, std::size_t> quadOf;
    for (const Rim& rim : rims) {
        quadOf[rim.edge] = quads.size();
        quads.push_back(ringQuad(&mesh, rim, copies));
    }

    // the edge rising at a corner is the one the next quad round the outline falls down
    std::vector<Index> ring;
    for (std::size_t index = 0; index < rims.size(); index++) {
        const auto next = quadOf.find(rims[index].after);
        if (next != quadOf.end()) {
            mesh.edge(quads[index].rising)->pair(quads[next->second].falling);
            mesh.edge(quads[next->second].falling)->pair(quads[index].rising);
        }
        ring.push_back(quads[index].face);
    }
    for (const Index quad : ring) {
        mesh.face(quad)->normal(newellNormal(mesh, quad));
    }
    return ring;
}

std::optional<std::vector<Index>> insetFaces(BRep & mesh, const std::vector<Index> & faces, float fraction) {
    for (const Index face : faces) {
        if (mesh.face(face) == nullptr) {
            return std::nullopt;
        }
    }
    std::vector<Index> ring;
    for (const Index face : faces) {
        const glm::vec3 middle = center(mesh, face);
        // a face alone is a region whose every corner is on its outline, so every corner it
        // holds after the extrude is a copy only it uses
        const std::vector<Index> quads = extrudeFaces(mesh, {face}).value_or(std::vector<Index>{});
        for (const Index edge : faceLoop(mesh, face)) {
            Vertex* corner = mesh.vertex(mesh.edge(edge)->vertex());
            corner->point(glm::mix(corner->point(), middle, fraction));
        }
        mesh.face(face)->normal(newellNormal(mesh, face));
        for (const Index quad : quads) {
            mesh.face(quad)->normal(newellNormal(mesh, quad));
            ring.push_back(quad);
        }
    }
    return ring;
}

};  // namespace v3d::brep
