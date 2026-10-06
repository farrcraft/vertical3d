/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Topology.h>

#include <algorithm>
#include <type_traits>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>

namespace {
/**
 * A unit quad in the z = 0 plane, wound counter-clockwise.
 **/
std::vector<glm::vec3> quad(float z) {
    std::vector<glm::vec3> vertices;
    vertices.push_back(glm::vec3(0.0f, 0.0f, z));
    vertices.push_back(glm::vec3(1.0f, 0.0f, z));
    vertices.push_back(glm::vec3(1.0f, 1.0f, z));
    vertices.push_back(glm::vec3(0.0f, 1.0f, z));
    return vertices;
}
};  // namespace

BOOST_AUTO_TEST_CASE(brep_empty_test) {
    v3d::brep::BRep mesh;

    BOOST_CHECK_EQUAL(mesh.vertexCount(), 0u);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 0u);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 0u);

    // out of range lookups are null rather than an overrun
    BOOST_CHECK(mesh.vertex(0) == nullptr);
    BOOST_CHECK(mesh.edge(0) == nullptr);
    BOOST_CHECK(mesh.face(0) == nullptr);

    // and an empty mesh bounds an empty box at the origin
    v3d::type::geometry::AABBox bound = mesh.bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(0.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_face_test) {
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    glm::vec3 normal(0.0f, 0.0f, 1.0f);
    mesh->addFace(quad(0.0f), normal);

    BOOST_CHECK_EQUAL(mesh->vertexCount(), 4u);
    BOOST_CHECK_EQUAL(mesh->edgeCount(), 4u);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 1u);

    // the face points at the first of its edges
    v3d::brep::Face* face = mesh->face(0);
    BOOST_REQUIRE(face != nullptr);
    BOOST_CHECK_EQUAL(face->edge(), 0u);
    BOOST_CHECK_EQUAL((face->normal() == normal), true);

    // whose half edges form a closed ring, all of them belonging to that face
    for (unsigned int index = 0; index < 4; ++index) {
        v3d::brep::HalfEdge* edge = mesh->edge(index);
        BOOST_REQUIRE(edge != nullptr);
        BOOST_CHECK_EQUAL(edge->face(), 0u);
        BOOST_CHECK_EQUAL(edge->next(), (index + 1) % 4);
        BOOST_CHECK_EQUAL(edge->vertex(), index);
    }

    // a lone face has nothing to pair its edges with, so they keep the sentinel
    BOOST_CHECK_EQUAL(mesh->edge(0)->pair(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(v3d::brep::BRep::INVALID_ID, v3d::brep::INVALID_ID);

    // the bound spans the quad
    v3d::type::geometry::AABBox bound = mesh->bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f, 0.0f, 0.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0f, 1.0f, 0.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_face_loop_test) {
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    mesh->addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // the loop walks the ring once and then stops
    const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, 0);
    BOOST_REQUIRE_EQUAL(loop.size(), 4u);

    // and each entry resolves to the vertex its half edge ends at
    glm::vec3 sum(0.0f);
    for (const v3d::brep::Index entry : loop) {
        sum += mesh->vertex(mesh->edge(entry)->vertex())->point();
    }

    // and center averages those vertices
    glm::vec3 middle = v3d::brep::center(*mesh, 0);
    BOOST_CHECK_CLOSE(middle[0], 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(middle[1], 0.5f, 0.01f);
    BOOST_CHECK_EQUAL(middle[2], 0.0f);
    BOOST_CHECK_EQUAL((middle == (sum / 4.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_shared_vertex_test) {
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    mesh->addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // a second face over the same four points reuses the vertices it already has, but
    // gets its own half edges
    mesh->addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 4u);
    BOOST_CHECK_EQUAL(mesh->edgeCount(), 8u);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 2u);
    BOOST_CHECK_EQUAL(mesh->face(1)->edge(), 4u);

    // a face somewhere else adds all of its own
    mesh->addFace(quad(2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 8u);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 3u);

    v3d::type::geometry::AABBox bound = mesh->bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f, 0.0f, 0.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0f, 1.0f, 2.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_identity_test) {
    v3d::brep::BRep first;
    v3d::brep::BRep second;

    // a mesh is a dag::Node, so it has an id for the selection model to key on
    BOOST_CHECK(first.id() != second.id());
    // which a copy would share with its original
    static_assert(!std::is_copy_constructible_v<v3d::brep::BRep>);
    static_assert(!std::is_copy_assignable_v<v3d::brep::BRep>);

    // and a dag::Transform, so its geometry is described about its own origin
    first.translation(glm::vec3(4.0f, 0.0f, 0.0f));
    glm::vec4 placed = first.matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_CLOSE(placed.x, 5.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(brep_selection_test) {
    v3d::brep::BRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    BOOST_CHECK_EQUAL(mesh.selected(), false);
    mesh.selected(true);
    BOOST_CHECK_EQUAL(mesh.selected(), true);

    mesh.vertex(0)->selected(true);
    mesh.edge(0)->selected(true);
    mesh.face(0)->selected(true);

    mesh.deselectComponents();

    BOOST_CHECK_EQUAL(mesh.vertex(0)->selected(), false);
    BOOST_CHECK_EQUAL(mesh.edge(0)->selected(), false);
    BOOST_CHECK_EQUAL(mesh.face(0)->selected(), false);
    // the object's own selection survives a component deselect
    BOOST_CHECK_EQUAL(mesh.selected(), true);
}

/**
 * A mesh built face by face holds only what it refers to, and one whose references reach past
 * what it holds - as a document edited by hand might - says which reference it is.
 **/
BOOST_AUTO_TEST_CASE(brep_validate_test) {
    v3d::brep::BRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    std::string problem;
    BOOST_CHECK(mesh.validate(&problem));
    BOOST_CHECK(problem.empty());

    mesh.edge(2)->next(40);
    BOOST_CHECK(!mesh.validate(&problem));
    BOOST_CHECK_EQUAL(problem, "edge 2 names something the mesh does not hold");

    mesh.edge(2)->next(3);
    mesh.face(0)->edge(v3d::brep::INVALID_ID);
    BOOST_CHECK(!mesh.validate(&problem));
    BOOST_CHECK_EQUAL(problem, "face 0 names an edge the mesh does not hold");
}

/**
 * A ring that does not close ends a face's loop after each edge has been walked once. A face
 * added after it, wound the other way, pairs with the edges whose ring is intact.
 **/
BOOST_AUTO_TEST_CASE(brep_open_ring_test) {
    v3d::brep::BRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // the last edge points back into the ring part way round, so it never reaches the first
    mesh.edge(3)->next(1);
    const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(mesh, 0);
    const std::vector<v3d::brep::Index> walked = {0, 1, 2, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(loop.begin(), loop.end(), walked.begin(), walked.end());
    BOOST_CHECK_LE(loop.size(), mesh.edgeCount());

    // the same square wound the other way, so its edges run opposite the first face's
    std::vector<glm::vec3> reversed = quad(0.0f);
    std::reverse(reversed.begin() + 1, reversed.end());
    mesh.addFace(reversed, glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_REQUIRE_EQUAL(mesh.faceCount(), 2u);
    BOOST_REQUIRE_EQUAL(mesh.edgeCount(), 8u);

    // edge 6 runs from vertex 3 to 2 and edge 7 from 2 to 1, opposite edges 3 and 2
    BOOST_CHECK_EQUAL(mesh.edge(6)->pair(), 3u);
    BOOST_CHECK_EQUAL(mesh.edge(3)->pair(), 6u);
    BOOST_CHECK_EQUAL(mesh.edge(7)->pair(), 2u);
    BOOST_CHECK_EQUAL(mesh.edge(2)->pair(), 7u);
    // the broken ring gives edge 0 no predecessor and edge 1 the wrong one, so neither pairs
    BOOST_CHECK_EQUAL(mesh.edge(0)->pair(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(mesh.edge(1)->pair(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(mesh.edge(4)->pair(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(mesh.edge(5)->pair(), v3d::brep::INVALID_ID);

    const std::vector<v3d::brep::Index> second = v3d::brep::faceLoop(mesh, 1);
    const std::vector<v3d::brep::Index> ring = {4, 5, 6, 7};
    BOOST_CHECK_EQUAL_COLLECTIONS(second.begin(), second.end(), ring.begin(), ring.end());
}

/**
 * A face's u runs along its first edge and v is square to it, both in its plane. On the top
 * of a unit cube they are x and y. On its +x side they are y and z.
 **/
BOOST_AUTO_TEST_CASE(brep_face_uv_of_a_cube_face_test) {
    v3d::brep::BRep mesh;
    mesh.addFace(quad(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    std::vector<glm::vec3> side;
    side.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    side.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
    side.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    side.push_back(glm::vec3(1.0f, 0.0f, 1.0f));
    mesh.addFace(side, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec3 u(0.0f);
    glm::vec3 v(0.0f);
    v3d::brep::faceUV(mesh, 0, &u, &v);
    BOOST_CHECK_SMALL(glm::length(u - glm::vec3(1.0f, 0.0f, 0.0f)), 1e-6f);
    BOOST_CHECK_SMALL(glm::length(v - glm::vec3(0.0f, 1.0f, 0.0f)), 1e-6f);

    v3d::brep::faceUV(mesh, 1, &u, &v);
    BOOST_CHECK_SMALL(glm::length(u - glm::vec3(0.0f, 1.0f, 0.0f)), 1e-6f);
    BOOST_CHECK_SMALL(glm::length(v - glm::vec3(0.0f, 0.0f, 1.0f)), 1e-6f);
}

/**
 * A face whose first two edges run along one line takes the next edge that turns, rather than
 * normalising a zero cross product. A face whose vertices all lie on a line leaves u and v as
 * they were.
 **/
BOOST_AUTO_TEST_CASE(brep_face_uv_of_a_degenerate_face_test) {
    v3d::brep::BRep mesh;
    std::vector<glm::vec3> straight;
    straight.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    straight.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    straight.push_back(glm::vec3(2.0f, 0.0f, 0.0f));
    straight.push_back(glm::vec3(2.0f, 1.0f, 0.0f));
    straight.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    mesh.addFace(straight, glm::vec3(0.0f, 0.0f, 1.0f));
    std::vector<glm::vec3> line;
    line.push_back(glm::vec3(0.0f, 0.0f, 5.0f));
    line.push_back(glm::vec3(1.0f, 0.0f, 5.0f));
    line.push_back(glm::vec3(2.0f, 0.0f, 5.0f));
    mesh.addFace(line, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 u(0.0f);
    glm::vec3 v(0.0f);
    v3d::brep::faceUV(mesh, 0, &u, &v);
    BOOST_CHECK_SMALL(glm::length(u - glm::vec3(1.0f, 0.0f, 0.0f)), 1e-6f);
    BOOST_CHECK_SMALL(glm::length(v - glm::vec3(0.0f, 1.0f, 0.0f)), 1e-6f);

    u = glm::vec3(7.0f);
    v = glm::vec3(9.0f);
    v3d::brep::faceUV(mesh, 1, &u, &v);
    BOOST_CHECK_EQUAL((u == glm::vec3(7.0f)), true);
    BOOST_CHECK_EQUAL((v == glm::vec3(9.0f)), true);
}
