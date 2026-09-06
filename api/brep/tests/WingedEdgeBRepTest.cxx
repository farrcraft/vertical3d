/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

#include <boost/test/unit_test.hpp>

#include "../WingedEdgeBRep.h"

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

BOOST_AUTO_TEST_CASE(winged_empty_test) {
    v3d::brep::WingedEdgeBRep mesh;

    BOOST_CHECK_EQUAL(mesh.vertexCount(), 0u);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 0u);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 0u);

    // out of range lookups are null rather than an overrun
    BOOST_CHECK(mesh.vertex(0) == nullptr);
    BOOST_CHECK(mesh.edge(0) == nullptr);
    BOOST_CHECK(mesh.face(0) == nullptr);

    const v3d::type::AABBox bound = mesh.bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(0.0f)), true);
}

/**
 * A face is its vertices, an edge between each consecutive pair, and one Face naming the
 * first of them.
 **/
BOOST_AUTO_TEST_CASE(winged_face_test) {
    v3d::brep::WingedEdgeBRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), true);

    BOOST_CHECK_EQUAL(mesh.vertexCount(), 4u);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 4u);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 1u);

    BOOST_REQUIRE(mesh.face(0) != nullptr);
    BOOST_CHECK_EQUAL((mesh.face(0)->normal() == glm::vec3(0.0f, 0.0f, 1.0f)), true);
}

/**
 * A vertex is welded rather than added twice, so a second face sharing an edge with the
 * first adds only the vertices that are new.
 **/
BOOST_AUTO_TEST_CASE(winged_shared_vertices_test) {
    v3d::brep::WingedEdgeBRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), true);
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), false);

    // the same four points, so the second face welds onto them
    BOOST_CHECK_EQUAL(mesh.vertexCount(), 4u);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 2u);
}

/**
 * The bound is the extent of the points, in the mesh's own space - the transform places it,
 * and bound() is taken before that.
 **/
BOOST_AUTO_TEST_CASE(winged_bound_test) {
    v3d::brep::WingedEdgeBRep mesh;
    mesh.addFace(quad(2.0f), glm::vec3(0.0f, 0.0f, 1.0f), true);

    const v3d::type::AABBox bound = mesh.bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0f, 0.0f, 2.0f)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0f, 1.0f, 2.0f)), true);
}

/**
 * An edge walk over a face reaches every edge of it and comes back round.
 **/
BOOST_AUTO_TEST_CASE(winged_edge_iterator_test) {
    v3d::brep::WingedEdgeBRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), true);

    v3d::brep::WingedEdgeBRep::edge_iterator it(&mesh, 0);
    BOOST_REQUIRE(*it != nullptr);
    BOOST_CHECK_EQUAL(it.brep(), &mesh);

    unsigned int walked = 0;
    for (; *it != nullptr && walked < 8; it++) {
        walked++;
    }
    // the walk ends rather than running away, and covers the face's four edges
    BOOST_CHECK_EQUAL(walked, 4u);
}

/**
 * A mesh is a dag::Node with a dag::Transform, the same as BRep, so it has an id and a
 * placement and its geometry is described about its own origin.
 **/
BOOST_AUTO_TEST_CASE(winged_is_a_dag_node_test) {
    v3d::brep::WingedEdgeBRep mesh;
    v3d::brep::WingedEdgeBRep other;

    BOOST_CHECK(mesh.id() != other.id());

    mesh.translation(glm::vec3(1.0f, 2.0f, 3.0f));
    BOOST_CHECK_EQUAL((mesh.translation() == glm::vec3(1.0f, 2.0f, 3.0f)), true);
}

/**
 * Selection is per mesh for the object and per component for the parts, which is the rule
 * the select mask depends on.
 **/
BOOST_AUTO_TEST_CASE(winged_selection_test) {
    v3d::brep::WingedEdgeBRep mesh;
    mesh.addFace(quad(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), true);

    BOOST_CHECK_EQUAL(mesh.selected(), false);
    mesh.selected(true);
    BOOST_CHECK_EQUAL(mesh.selected(), true);

    // the mesh's own flag is not its components'
    BOOST_REQUIRE(mesh.edge(0) != nullptr);
    BOOST_CHECK_EQUAL(mesh.edge(0)->selected(), false);
}
