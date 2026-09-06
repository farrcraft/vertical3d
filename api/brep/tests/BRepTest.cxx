/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../BRep.h"

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
    v3d::type::AABBox bound = mesh.bound();
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
    v3d::type::AABBox bound = mesh->bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f, 0.0f, 0.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0f, 1.0f, 0.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_iterator_test) {
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    mesh->addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // the edge iterator walks the ring once and then stops
    unsigned int edges = 0;
    for (v3d::brep::BRep::edge_iterator it(mesh, 0); *it != nullptr; it++) {
        ++edges;
        BOOST_REQUIRE(edges <= 4);
    }
    BOOST_CHECK_EQUAL(edges, 4u);

    // the vertex iterator walks the same ring, resolving each edge to its vertex
    unsigned int vertices = 0;
    glm::vec3 sum(0.0f);
    for (v3d::brep::BRep::vertex_iterator it(mesh, 0); *it != nullptr; it++) {
        sum += (*it)->point();
        ++vertices;
        BOOST_REQUIRE(vertices <= 4);
    }
    BOOST_CHECK_EQUAL(vertices, 4u);

    // which is what center averages
    glm::vec3 middle = v3d::brep::center(mesh, 0);
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

    v3d::type::AABBox bound = mesh->bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f, 0.0f, 0.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0f, 1.0f, 2.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_split_edge_test) {
    v3d::brep::BRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // splitting an edge adds the point as a vertex and a half edge that carries it,
    // spliced into the ring after the edge it split
    uint64_t next = mesh.edge(0)->next();
    mesh.splitEdge(0, glm::vec3(0.5f, 0.0f, 0.0f));
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 5u);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 5u);
    BOOST_CHECK_EQUAL(mesh.edge(0)->next(), 4u);
    BOOST_CHECK_EQUAL(mesh.edge(4)->next(), next);
    BOOST_CHECK_EQUAL(mesh.edge(4)->vertex(), 4u);
    BOOST_CHECK_EQUAL((mesh.vertex(4)->point() == glm::vec3(0.5f, 0.0f, 0.0f)), true);
}

BOOST_AUTO_TEST_CASE(brep_identity_test) {
    v3d::brep::BRep first;
    v3d::brep::BRep second;

    // a mesh is a dag::Node, which is what gives the selection model something to key on
    BOOST_CHECK(first.id() != second.id());

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
