/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Operations.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A unit cube from the origin to (1, 1, 1), each face wound clockwise seen from outside, so
 * every half edge finds its pair. Faces 0 to 5 are bottom, top, front (y = 0), back, left
 * (x = 0) and right.
 **/
void cube(v3d::brep::BRep* mesh) {
    const glm::vec3 origin(0.0F, 0.0F, 0.0F);
    const glm::vec3 x(1.0F, 0.0F, 0.0F);
    const glm::vec3 y(0.0F, 1.0F, 0.0F);
    const glm::vec3 z(0.0F, 0.0F, 1.0F);
    mesh->addFace({origin, x, x + y, y}, -z);
    mesh->addFace({z, y + z, x + y + z, x + z}, z);
    mesh->addFace({origin, z, x + z, x}, -y);
    mesh->addFace({y, x + y, x + y + z, y + z}, y);
    mesh->addFace({origin, y, y + z, z}, -x);
    mesh->addFace({x, x + z, x + y + z, x + y}, x);
}

/**
 * Whether two snapshots hold the same mesh, index for index: every vertex's point, every half
 * edge's four references and every face's normal and edge.
 **/
bool same(const v3d::brep::BRep::Snapshot& left, const v3d::brep::BRep::Snapshot& right) {
    if (left.vertices.size() != right.vertices.size() || left.edges.size() != right.edges.size() ||
        left.faces.size() != right.faces.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.vertices.size(); index++) {
        if (left.vertices[index].point() != right.vertices[index].point()) {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.edges.size(); index++) {
        const v3d::brep::HalfEdge& one = left.edges[index];
        const v3d::brep::HalfEdge& other = right.edges[index];
        if (one.vertex() != other.vertex() || one.face() != other.face() || one.pair() != other.pair() ||
            one.next() != other.next()) {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.faces.size(); index++) {
        if (left.faces[index].normal() != right.faces[index].normal() || left.faces[index].edge() != right.faces[index].edge()) {
            return false;
        }
    }
    return true;
}

/**
 * Whether the mesh passes the strict check, reporting why when it does not.
 **/
void sound(const v3d::brep::BRep& mesh) {
    std::string problem;
    BOOST_CHECK_MESSAGE(mesh.validateStrict(&problem), problem);
}

BOOST_AUTO_TEST_CASE(brep_snapshot_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();

    // a restore puts back every array whole, including what was appended since
    mesh.vertex(0)->point(glm::vec3(5.0F));
    mesh.addFace({glm::vec3(3.0F, 0.0F, 0.0F), glm::vec3(4.0F, 0.0F, 0.0F), glm::vec3(4.0F, 1.0F, 0.0F)},
        glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_CHECK(!same(mesh.snapshot(), before));
    mesh.restore(before);
    BOOST_CHECK(same(mesh.snapshot(), before));
    BOOST_CHECK_EQUAL(mesh.faceCount(), 6U);
}

BOOST_AUTO_TEST_CASE(brep_split_edge_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    const v3d::brep::Index pair = mesh.edge(0)->pair();
    const v3d::brep::Index face = mesh.edge(0)->face();
    const v3d::brep::Index other = mesh.edge(pair)->face();
    const std::optional<v3d::brep::Segment> span = v3d::brep::loopSegment(mesh, v3d::brep::faceLoop(mesh, face), 0);
    BOOST_REQUIRE(span.has_value());
    const glm::vec3 halfway = span.has_value() ? (span->from + span->to) * 0.5F : glm::vec3(0.0F);

    const std::optional<v3d::brep::Index> middle = v3d::brep::splitEdge(mesh, 0);
    BOOST_REQUIRE(middle);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 9U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 26U);
    BOOST_CHECK_SMALL(glm::distance(mesh.vertex(*middle)->point(), halfway), 0.0001F);

    // the half edge keeps its index and ends at the new vertex, and both faces gain a corner
    BOOST_CHECK_EQUAL(mesh.edge(0)->vertex(), *middle);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, face).size(), 5U);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, other).size(), 5U);
    BOOST_CHECK_EQUAL(v3d::brep::facesAround(mesh, *middle).size(), 2U);

    // the old arrays come back index for index
    const v3d::brep::BRep::Snapshot after = mesh.snapshot();
    mesh.restore(before);
    BOOST_CHECK(same(mesh.snapshot(), before));
    mesh.restore(after);
    BOOST_CHECK(same(mesh.snapshot(), after));

    BOOST_CHECK(!v3d::brep::splitEdge(mesh, 400));
}

BOOST_AUTO_TEST_CASE(brep_split_boundary_edge_test) {
    // an edge on an open boundary has no pair, so only its one half is split
    v3d::brep::BRep mesh;
    mesh.addFace({glm::vec3(0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F)},
        glm::vec3(0.0F, 0.0F, 1.0F));
    const std::optional<v3d::brep::Index> middle = v3d::brep::splitEdge(mesh, 2);
    BOOST_REQUIRE(middle);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 5U);
    BOOST_CHECK_EQUAL(mesh.edge(4)->pair(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, 0).size(), 5U);
}

BOOST_AUTO_TEST_CASE(brep_split_face_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    // the bottom face, between its corner at the origin and the one across from it
    const v3d::brep::Index first = 0;
    v3d::brep::Index second = v3d::brep::INVALID_ID;
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        if (mesh.vertex(id)->point() == glm::vec3(1.0F, 1.0F, 0.0F)) {
            second = id;
        }
    }
    const glm::vec3 normal = mesh.face(0)->normal();

    const std::optional<v3d::brep::Index> added = v3d::brep::splitFace(mesh, 0, first, second);
    BOOST_REQUIRE(added);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 7U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 26U);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, 0).size(), 3U);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, *added).size(), 3U);

    // both halves keep the face's winding, so both point the way it did
    BOOST_CHECK_SMALL(glm::distance(mesh.face(0)->normal(), normal), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(mesh.face(*added)->normal(), normal), 0.0001F);

    const v3d::brep::BRep::Snapshot after = mesh.snapshot();
    mesh.restore(before);
    BOOST_CHECK(same(mesh.snapshot(), before));
    mesh.restore(after);
    BOOST_CHECK(same(mesh.snapshot(), after));
}

BOOST_AUTO_TEST_CASE(brep_split_face_refused_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    v3d::brep::Index along = v3d::brep::INVALID_ID;
    v3d::brep::Index above = v3d::brep::INVALID_ID;
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        if (mesh.vertex(id)->point() == glm::vec3(1.0F, 0.0F, 0.0F)) {
            along = id;
        }
        if (mesh.vertex(id)->point() == glm::vec3(0.0F, 0.0F, 1.0F)) {
            above = id;
        }
    }

    // corners next to each other are already joined, a corner cannot be joined to itself, and
    // a vertex that is not a corner of the face cannot be joined at all
    BOOST_CHECK(!v3d::brep::splitFace(mesh, 0, 0, along));
    BOOST_CHECK(!v3d::brep::splitFace(mesh, 0, 0, 0));
    BOOST_CHECK(!v3d::brep::splitFace(mesh, 0, 0, above));
    BOOST_CHECK(!v3d::brep::splitFace(mesh, 9, 0, along));
    BOOST_CHECK(same(mesh.snapshot(), before));
}

/**
 * Restore the arrays from before an operation and then from after it, and check that each comes
 * back index for index.
 **/
void restores(v3d::brep::BRep* mesh, const v3d::brep::BRep::Snapshot& before) {
    const v3d::brep::BRep::Snapshot after = mesh->snapshot();
    mesh->restore(before);
    BOOST_CHECK(same(mesh->snapshot(), before));
    mesh->restore(after);
    BOOST_CHECK(same(mesh->snapshot(), after));
}

BOOST_AUTO_TEST_CASE(brep_extrude_face_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();

    const std::vector<v3d::brep::Index> ring = v3d::brep::extrudeFaces(mesh, {1}).value_or(std::vector<v3d::brep::Index>{});
    sound(mesh);
    BOOST_CHECK_EQUAL(ring.size(), 4U);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 12U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 40U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 10U);
    restores(&mesh, before);

    // the top keeps its index and now uses only the copies
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, 1)) {
        BOOST_CHECK_GE(mesh.edge(edge)->vertex(), 8U);
    }

    // pulled up, the ring's quads are walls whose normals point away from the cube, so the ring
    // is wound the way the faces around it are
    std::vector<v3d::brep::Index> copies;
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, 1)) {
        copies.push_back(mesh.edge(edge)->vertex());
    }
    for (const v3d::brep::Index copy : copies) {
        mesh.vertex(copy)->point(mesh.vertex(copy)->point() + glm::vec3(0.0F, 0.0F, 1.0F));
    }
    sound(mesh);
    for (const v3d::brep::Index quad : ring) {
        const glm::vec3 normal = v3d::brep::newellNormal(mesh, quad);
        BOOST_CHECK_SMALL(normal.z, 0.0001F);
        BOOST_CHECK_GT(glm::dot(normal, v3d::brep::center(mesh, quad) - glm::vec3(0.5F, 0.5F, 1.5F)), 0.0F);
    }
    BOOST_CHECK_SMALL(glm::distance(v3d::brep::newellNormal(mesh, 1), glm::vec3(0.0F, 0.0F, 1.0F)), 0.0001F);
}

BOOST_AUTO_TEST_CASE(brep_extrude_region_test) {
    // the bottom and the front share an edge, so they extrude as one region with a ring of six
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    const std::vector<v3d::brep::Index> ring = v3d::brep::extrudeFaces(mesh, {0, 2}).value_or(std::vector<v3d::brep::Index>{});
    sound(mesh);
    BOOST_CHECK_EQUAL(ring.size(), 6U);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 14U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 12U);

    // the edge between them still joins them
    bool joined = false;
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, 0)) {
        joined = joined || mesh.edge(mesh.edge(edge)->pair())->face() == 2;
    }
    BOOST_CHECK(joined);
    restores(&mesh, before);

    // the bottom and the top share nothing, so they are two regions with a ring each
    mesh.restore(before);
    const std::vector<v3d::brep::Index> apart = v3d::brep::extrudeFaces(mesh, {0, 1}).value_or(std::vector<v3d::brep::Index>{});
    sound(mesh);
    BOOST_CHECK_EQUAL(apart.size(), 8U);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 16U);
}

BOOST_AUTO_TEST_CASE(brep_extrude_open_face_test) {
    // a face alone has no faces beside it, so the ring's lower edges are a new boundary
    v3d::brep::BRep mesh;
    mesh.addFace({glm::vec3(0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F)},
        glm::vec3(0.0F, 0.0F, 1.0F));
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    const std::optional<std::vector<v3d::brep::Index>> ring = v3d::brep::extrudeFaces(mesh, {0});
    BOOST_REQUIRE(ring);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 8U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 20U);
    std::size_t open = 0;
    for (v3d::brep::Index edge = 0; edge < mesh.edgeCount(); edge++) {
        open += mesh.edge(edge)->pair() == v3d::brep::INVALID_ID ? 1 : 0;
    }
    BOOST_CHECK_EQUAL(open, 4U);
    restores(&mesh, before);
}

BOOST_AUTO_TEST_CASE(brep_inset_face_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    const std::vector<v3d::brep::Index> ring = v3d::brep::insetFaces(mesh, {1}, 0.25F).value_or(std::vector<v3d::brep::Index>{});
    BOOST_CHECK_EQUAL(ring.size(), 4U);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 12U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 10U);

    // each corner of the top moved a quarter of the way to its centre, and the ring lies in the
    // top's plane, facing the way it does
    const glm::vec3 middle(0.5F, 0.5F, 1.0F);
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, 1)) {
        BOOST_CHECK_SMALL(glm::distance(mesh.vertex(mesh.edge(edge)->vertex())->point(), middle) - (0.75F * glm::length(glm::vec3(0.5F, 0.5F, 0.0F))),
            0.0001F);
    }
    for (const v3d::brep::Index quad : ring) {
        BOOST_CHECK_SMALL(glm::distance(mesh.face(quad)->normal(), glm::vec3(0.0F, 0.0F, 1.0F)), 0.0001F);
    }
    restores(&mesh, before);
}

BOOST_AUTO_TEST_CASE(brep_inset_adjacent_faces_test) {
    // each face is inset alone, so the edge the two share stays between their rings
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    BOOST_REQUIRE(v3d::brep::insetFaces(mesh, {0, 2}, 0.25F));
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 16U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 56U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 14U);
    restores(&mesh, before);
}

/**
 * The index of a mesh's vertex at a point.
 **/
v3d::brep::Index at(const v3d::brep::BRep& mesh, const glm::vec3& point) {
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        if (glm::distance(mesh.vertex(id)->point(), point) < 0.0001F) {
            return id;
        }
    }
    return v3d::brep::INVALID_ID;
}

BOOST_AUTO_TEST_CASE(brep_merge_edge_test) {
    // the two ends of the cube's edge along x from the origin become one at its middle, and the
    // bottom and the front, which shared the edge, become triangles
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    const v3d::brep::Index end = at(mesh, glm::vec3(1.0F, 0.0F, 0.0F));

    const std::optional<v3d::brep::Index> merged = v3d::brep::mergeVertices(mesh, {0, end});
    BOOST_REQUIRE(merged);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 7U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 22U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 6U);
    BOOST_CHECK_SMALL(glm::distance(mesh.vertex(merged.value_or(0))->point(), glm::vec3(0.5F, 0.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, 0).size(), 3U);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, 2).size(), 3U);
    BOOST_CHECK_EQUAL(v3d::brep::facesAround(mesh, merged.value_or(0)).size(), 4U);
    BOOST_CHECK_SMALL(glm::distance(mesh.face(0)->normal(), glm::vec3(0.0F, 0.0F, -1.0F)), 0.0001F);
    restores(&mesh, before);
}

BOOST_AUTO_TEST_CASE(brep_merge_face_test) {
    // the top's four corners become one, so the top goes and the cube becomes a pyramid, with
    // the faces after the top renumbered down by one
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    std::vector<v3d::brep::Index> corners;
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, 1)) {
        corners.push_back(mesh.edge(edge)->vertex());
    }

    const std::optional<v3d::brep::Index> merged = v3d::brep::mergeVertices(mesh, corners);
    BOOST_REQUIRE(merged);
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 5U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 16U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 5U);
    BOOST_CHECK_SMALL(glm::distance(mesh.vertex(merged.value_or(0))->point(), glm::vec3(0.5F, 0.5F, 1.0F)), 0.0001F);
    BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, 0).size(), 4U);
    for (v3d::brep::Index face = 1; face < mesh.faceCount(); face++) {
        BOOST_CHECK_EQUAL(v3d::brep::faceLoop(mesh, face).size(), 3U);
    }
    // the front, once face 2, is face 1, and still faces the way it did
    BOOST_CHECK_SMALL(mesh.face(1)->normal().x, 0.0001F);
    BOOST_CHECK_LT(mesh.face(1)->normal().y, 0.0F);
    restores(&mesh, before);
}

BOOST_AUTO_TEST_CASE(brep_merge_weld_test) {
    // two quads a gap apart, merged across the gap at both ends, share the edge between them
    v3d::brep::BRep mesh;
    mesh.addFace({glm::vec3(0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F)},
        glm::vec3(0.0F, 0.0F, 1.0F));
    mesh.addFace({glm::vec3(1.5F, 0.0F, 0.0F), glm::vec3(1.5F, 1.0F, 0.0F), glm::vec3(2.0F, 1.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F)},
        glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_REQUIRE(v3d::brep::mergeVertices(mesh, {at(mesh, glm::vec3(1.0F, 0.0F, 0.0F)), at(mesh, glm::vec3(1.5F, 0.0F, 0.0F))}));
    BOOST_REQUIRE(v3d::brep::mergeVertices(mesh, {at(mesh, glm::vec3(1.0F, 1.0F, 0.0F)), at(mesh, glm::vec3(1.5F, 1.0F, 0.0F))}));
    sound(mesh);
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 6U);
    std::size_t paired = 0;
    for (v3d::brep::Index edge = 0; edge < mesh.edgeCount(); edge++) {
        paired += mesh.edge(edge)->pair() == v3d::brep::INVALID_ID ? 0 : 1;
    }
    BOOST_CHECK_EQUAL(paired, 2U);
}

BOOST_AUTO_TEST_CASE(brep_merge_refused_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();

    // the bottom's opposite corners: the bottom would meet itself, and its two sides would each
    // share an edge with two faces
    BOOST_CHECK(!v3d::brep::mergeVertices(mesh, {0, at(mesh, glm::vec3(1.0F, 1.0F, 0.0F))}));
    BOOST_CHECK(!v3d::brep::mergeVertices(mesh, {0}));
    BOOST_CHECK(!v3d::brep::mergeVertices(mesh, {0, 0}));
    BOOST_CHECK(!v3d::brep::mergeVertices(mesh, {0, 40}));
    BOOST_CHECK(same(mesh.snapshot(), before));
}

BOOST_AUTO_TEST_CASE(brep_extrude_refused_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    BOOST_CHECK(!v3d::brep::extrudeFaces(mesh, {0, 40}));
    BOOST_CHECK(!v3d::brep::insetFaces(mesh, {0, 9}, 0.25F));
    BOOST_CHECK(same(mesh.snapshot(), before));
}

};  // namespace
