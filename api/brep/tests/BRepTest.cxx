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
#include <api/type/geometry/AABBox.h>

#include <algorithm>
#include <cstdint>
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
    vertices.push_back(glm::vec3(0.0F, 0.0F, z));
    vertices.push_back(glm::vec3(1.0F, 0.0F, z));
    vertices.push_back(glm::vec3(1.0F, 1.0F, z));
    vertices.push_back(glm::vec3(0.0F, 1.0F, z));
    return vertices;
}

BOOST_AUTO_TEST_CASE(brep_empty_test) {
    v3d::brep::BRep mesh;

    BOOST_CHECK_EQUAL(mesh.vertexCount(), 0U);
    BOOST_CHECK_EQUAL(mesh.edgeCount(), 0U);
    BOOST_CHECK_EQUAL(mesh.faceCount(), 0U);

    // out of range lookups are null rather than an overrun
    BOOST_CHECK(mesh.vertex(0) == nullptr);
    BOOST_CHECK(mesh.edge(0) == nullptr);
    BOOST_CHECK(mesh.face(0) == nullptr);

    // and an empty mesh bounds an empty box at the origin
    const v3d::type::geometry::AABBox bound = mesh.bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0F)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(0.0F)), true);
}

BOOST_AUTO_TEST_CASE(brep_face_test) {
    const boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    const glm::vec3 normal(0.0F, 0.0F, 1.0F);
    mesh->addFace(quad(0.0F), normal);

    BOOST_CHECK_EQUAL(mesh->vertexCount(), 4U);
    BOOST_CHECK_EQUAL(mesh->edgeCount(), 4U);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 1U);

    // the face points at the first of its edges
    const v3d::brep::Face* face = mesh->face(0);
    BOOST_REQUIRE(face != nullptr);
    BOOST_CHECK_EQUAL(face->edge(), 0U);
    BOOST_CHECK_EQUAL((face->normal() == normal), true);

    // whose half edges form a closed ring, all of them belonging to that face
    for (unsigned int index = 0; index < 4; ++index) {
        const v3d::brep::HalfEdge* edge = mesh->edge(index);
        BOOST_REQUIRE(edge != nullptr);
        BOOST_CHECK_EQUAL(edge->face(), 0U);
        BOOST_CHECK_EQUAL(edge->next(), (index + 1) % 4);
        BOOST_CHECK_EQUAL(edge->vertex(), index);
    }

    // a lone face has nothing to pair its edges with, so they keep the sentinel
    BOOST_CHECK_EQUAL(mesh->edge(0)->pair(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(v3d::brep::BRep::INVALID_ID, v3d::brep::INVALID_ID);

    // the bound spans the quad
    const v3d::type::geometry::AABBox bound = mesh->bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0F, 0.0F, 0.0F)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0F, 1.0F, 0.0F)), true);
}

BOOST_AUTO_TEST_CASE(brep_face_loop_test) {
    const boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    mesh->addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

    // the loop walks the ring once and then stops
    const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, 0);
    BOOST_REQUIRE_EQUAL(loop.size(), 4U);

    // and each entry resolves to the vertex its half edge ends at
    glm::vec3 sum(0.0F);
    for (const v3d::brep::Index entry : loop) {
        sum += mesh->vertex(mesh->edge(entry)->vertex())->point();
    }

    // and center averages those vertices
    glm::vec3 middle = v3d::brep::center(*mesh, 0);
    BOOST_CHECK_CLOSE(middle[0], 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(middle[1], 0.5F, 0.01F);
    BOOST_CHECK_EQUAL(middle[2], 0.0F);
    BOOST_CHECK_EQUAL((middle == (sum / 4.0F)), true);
}

BOOST_AUTO_TEST_CASE(brep_shared_vertex_test) {
    const boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    mesh->addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

    // a second face over the same four points reuses the vertices it already has, but
    // gets its own half edges
    mesh->addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, -1.0F));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 4U);
    BOOST_CHECK_EQUAL(mesh->edgeCount(), 8U);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 2U);
    BOOST_CHECK_EQUAL(mesh->face(1)->edge(), 4U);

    // a face somewhere else adds all of its own
    mesh->addFace(quad(2.0F), glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 8U);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 3U);

    const v3d::type::geometry::AABBox bound = mesh->bound();
    BOOST_CHECK_EQUAL((bound.min() == glm::vec3(0.0F, 0.0F, 0.0F)), true);
    BOOST_CHECK_EQUAL((bound.max() == glm::vec3(1.0F, 1.0F, 2.0F)), true);
}

BOOST_AUTO_TEST_CASE(brep_copy_test) {
    v3d::brep::BRep original;
    original.addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

    // a mesh is geometry alone, so a copy is a separate mesh with the same parts
    v3d::brep::BRep copy = original;
    BOOST_CHECK_EQUAL(copy.vertexCount(), original.vertexCount());
    copy.vertex(0)->point(glm::vec3(9.0F, 0.0F, 0.0F));
    BOOST_CHECK(original.vertex(0)->point() != glm::vec3(9.0F, 0.0F, 0.0F));
}

/**
 * A mesh built face by face refers only to what it holds. A mesh with a reference past what it
 * holds, as a hand-edited document might have, fails validation and names that reference.
 **/
BOOST_AUTO_TEST_CASE(brep_validate_test) {
    v3d::brep::BRep mesh;
    mesh.addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));
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
    mesh.addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));
    // the last edge points back into the ring part way round, so it never reaches the first
    mesh.edge(3)->next(1);
    const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(mesh, 0);
    const std::vector<v3d::brep::Index> walked = {0, 1, 2, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(loop.begin(), loop.end(), walked.begin(), walked.end());
    BOOST_CHECK_LE(loop.size(), mesh.edgeCount());

    // the same square wound the other way, so its edges run opposite the first face's
    std::vector<glm::vec3> reversed = quad(0.0F);
    std::reverse(reversed.begin() + 1, reversed.end());
    mesh.addFace(reversed, glm::vec3(0.0F, 0.0F, -1.0F));
    BOOST_REQUIRE_EQUAL(mesh.faceCount(), 2U);
    BOOST_REQUIRE_EQUAL(mesh.edgeCount(), 8U);

    // edge 6 runs from vertex 3 to 2 and edge 7 from 2 to 1, opposite edges 3 and 2
    BOOST_CHECK_EQUAL(mesh.edge(6)->pair(), 3U);
    BOOST_CHECK_EQUAL(mesh.edge(3)->pair(), 6U);
    BOOST_CHECK_EQUAL(mesh.edge(7)->pair(), 2U);
    BOOST_CHECK_EQUAL(mesh.edge(2)->pair(), 7U);
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
    mesh.addFace(quad(1.0F), glm::vec3(0.0F, 0.0F, 1.0F));
    std::vector<glm::vec3> side;
    side.push_back(glm::vec3(1.0F, 0.0F, 0.0F));
    side.push_back(glm::vec3(1.0F, 1.0F, 0.0F));
    side.push_back(glm::vec3(1.0F, 1.0F, 1.0F));
    side.push_back(glm::vec3(1.0F, 0.0F, 1.0F));
    mesh.addFace(side, glm::vec3(1.0F, 0.0F, 0.0F));

    glm::vec3 u(0.0F);
    glm::vec3 v(0.0F);
    v3d::brep::faceUV(mesh, 0, &u, &v);
    BOOST_CHECK_SMALL(glm::length(u - glm::vec3(1.0F, 0.0F, 0.0F)), 1e-6F);
    BOOST_CHECK_SMALL(glm::length(v - glm::vec3(0.0F, 1.0F, 0.0F)), 1e-6F);

    v3d::brep::faceUV(mesh, 1, &u, &v);
    BOOST_CHECK_SMALL(glm::length(u - glm::vec3(0.0F, 1.0F, 0.0F)), 1e-6F);
    BOOST_CHECK_SMALL(glm::length(v - glm::vec3(0.0F, 0.0F, 1.0F)), 1e-6F);
}

/**
 * A face whose first two edges run along one line takes the next edge that turns, rather than
 * normalising a zero cross product. A face whose vertices all lie on a line leaves u and v as
 * they were.
 **/
BOOST_AUTO_TEST_CASE(brep_face_uv_of_a_degenerate_face_test) {
    v3d::brep::BRep mesh;
    std::vector<glm::vec3> straight;
    straight.push_back(glm::vec3(0.0F, 0.0F, 0.0F));
    straight.push_back(glm::vec3(1.0F, 0.0F, 0.0F));
    straight.push_back(glm::vec3(2.0F, 0.0F, 0.0F));
    straight.push_back(glm::vec3(2.0F, 1.0F, 0.0F));
    straight.push_back(glm::vec3(0.0F, 1.0F, 0.0F));
    mesh.addFace(straight, glm::vec3(0.0F, 0.0F, 1.0F));
    std::vector<glm::vec3> line;
    line.push_back(glm::vec3(0.0F, 0.0F, 5.0F));
    line.push_back(glm::vec3(1.0F, 0.0F, 5.0F));
    line.push_back(glm::vec3(2.0F, 0.0F, 5.0F));
    mesh.addFace(line, glm::vec3(0.0F, 0.0F, 1.0F));

    glm::vec3 u(0.0F);
    glm::vec3 v(0.0F);
    v3d::brep::faceUV(mesh, 0, &u, &v);
    BOOST_CHECK_SMALL(glm::length(u - glm::vec3(1.0F, 0.0F, 0.0F)), 1e-6F);
    BOOST_CHECK_SMALL(glm::length(v - glm::vec3(0.0F, 1.0F, 0.0F)), 1e-6F);

    u = glm::vec3(7.0F);
    v = glm::vec3(9.0F);
    v3d::brep::faceUV(mesh, 1, &u, &v);
    BOOST_CHECK_EQUAL((u == glm::vec3(7.0F)), true);
    BOOST_CHECK_EQUAL((v == glm::vec3(9.0F)), true);
}

/**
 * A unit cube from the origin to (1, 1, 1), each face wound clockwise seen from outside, so
 * every half edge finds its pair. Faces 0 to 5 are bottom, top, front (y = 0), back, left
 * (x = 0) and right. Vertex 0 is the corner at the origin.
 **/
v3d::brep::BRep* cube(v3d::brep::BRep* mesh) {
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
    return mesh;
}

BOOST_AUTO_TEST_CASE(brep_strict_cube_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    BOOST_REQUIRE_EQUAL(mesh.edgeCount(), 24U);
    std::string problem;
    BOOST_CHECK(mesh.validateStrict(&problem));
    BOOST_CHECK(problem.empty());
    for (v3d::brep::Index id = 0; id < mesh.edgeCount(); id++) {
        BOOST_CHECK(mesh.edge(id)->pair() != v3d::brep::INVALID_ID);
    }

    // an open mesh has a boundary, and an edge with no pair is not a fault
    v3d::brep::BRep open;
    open.addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_CHECK(open.validateStrict(nullptr));
}

BOOST_AUTO_TEST_CASE(brep_strict_pair_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    const v3d::brep::Index pair = mesh.edge(0)->pair();
    mesh.edge(pair)->pair(v3d::brep::INVALID_ID);
    std::string problem;
    BOOST_CHECK(mesh.validate(&problem));
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "edge 0's pair " + std::to_string(pair) + " does not pair it back");

    // an edge paired with itself is no pair
    mesh.edge(pair)->pair(0);
    mesh.edge(5)->pair(5);
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "edge 5's pair 5 does not pair it back");
}

BOOST_AUTO_TEST_CASE(brep_strict_loop_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    // the last edge of the first face points back into the ring part way round
    mesh.edge(3)->next(1);
    std::string problem;
    BOOST_CHECK(mesh.validate(&problem));
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "face 0's loop does not close");

    // and a chain that runs off into nothing does not close either
    mesh.edge(3)->next(v3d::brep::INVALID_ID);
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "face 0's loop does not close");
}

BOOST_AUTO_TEST_CASE(brep_strict_face_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    mesh.edge(1)->face(2);
    std::string problem;
    BOOST_CHECK(mesh.validate(&problem));
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "edge 1 is in face 0's loop and names face 2");

    mesh.edge(1)->face(v3d::brep::INVALID_ID);
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "edge 1 is in face 0's loop and names face none");
}

BOOST_AUTO_TEST_CASE(brep_strict_span_test) {
    // a face that goes out to a vertex and comes straight back runs twice between the same
    // two vertices. Its half edges pair with each other, so only this check finds it
    v3d::brep::BRep mesh;
    const glm::vec3 first(0.0F, 0.0F, 0.0F);
    const glm::vec3 second(1.0F, 0.0F, 0.0F);
    const glm::vec3 third(1.0F, 1.0F, 0.0F);
    mesh.addFace({first, second, third, second}, glm::vec3(0.0F, 0.0F, 1.0F));
    std::string problem;
    BOOST_CHECK(mesh.validate(&problem));
    BOOST_CHECK(!mesh.validateStrict(&problem));
    BOOST_CHECK_EQUAL(problem, "face 0 has two edges between vertices 0 and 1");
}

BOOST_AUTO_TEST_CASE(brep_walk_corner_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    BOOST_REQUIRE(mesh.vertex(0)->point() == glm::vec3(0.0F, 0.0F, 0.0F));

    // bottom, front and left meet at the origin
    const std::vector<v3d::brep::Index> faces = v3d::brep::facesAround(mesh, 0);
    const std::vector<v3d::brep::Index> expected = {0, 2, 4};
    BOOST_CHECK_EQUAL_COLLECTIONS(faces.begin(), faces.end(), expected.begin(), expected.end());

    // three half edges leave the corner, one along each axis, each in one of those faces
    const std::vector<v3d::brep::Index> leaving = v3d::brep::edgesFrom(mesh, 0);
    BOOST_REQUIRE_EQUAL(leaving.size(), 3U);
    glm::vec3 sum(0.0F);
    for (const v3d::brep::Index id : leaving) {
        const glm::vec3 end = mesh.vertex(mesh.edge(id)->vertex())->point();
        BOOST_CHECK_CLOSE(glm::length(end), 1.0F, 0.001F);
        sum += end;
        BOOST_CHECK(std::find(faces.begin(), faces.end(), mesh.edge(id)->face()) != faces.end());
    }
    BOOST_CHECK(sum == glm::vec3(1.0F, 1.0F, 1.0F));

    // a vertex the mesh does not hold has nothing round it
    BOOST_CHECK(v3d::brep::facesAround(mesh, 99).empty());
    BOOST_CHECK(v3d::brep::edgesFrom(mesh, 99).empty());
}

BOOST_AUTO_TEST_CASE(brep_newell_normal_test) {
    v3d::brep::BRep mesh;
    cube(&mesh);
    // every face of the cube points out of it
    const glm::vec3 outward[6] = {
        glm::vec3(0.0F, 0.0F, -1.0F), glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, -1.0F, 0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F) };
    for (v3d::brep::Index face = 0; face < 6; face++) {
        BOOST_CHECK_SMALL(glm::length(v3d::brep::newellNormal(mesh, face) - outward[face]), 1e-6F);
    }

    // a quad with one corner lifted out of its plane still gets a unit normal close to the
    // plane's, tipped away from the lifted corner. It is wound clockwise seen from above
    v3d::brep::BRep bent;
    bent.addFace({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 1.0F, 0.2F),
        glm::vec3(1.0F, 0.0F, 0.0F)}, glm::vec3(0.0F, 0.0F, 1.0F));
    const glm::vec3 normal = v3d::brep::newellNormal(bent, 0);
    BOOST_CHECK_CLOSE(glm::length(normal), 1.0F, 0.001F);
    BOOST_CHECK_GT(normal.z, 0.98F);
    BOOST_CHECK_LT(normal.x, 0.0F);
    BOOST_CHECK_LT(normal.y, 0.0F);

    // a face on a line has no area and no normal
    v3d::brep::BRep line;
    line.addFace({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F)},
        glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_CHECK(v3d::brep::newellNormal(line, 0) == glm::vec3(0.0F));
    BOOST_CHECK(v3d::brep::newellNormal(line, 7) == glm::vec3(0.0F));
}

BOOST_AUTO_TEST_CASE(brep_changes_test) {
    v3d::brep::BRep mesh;
    BOOST_CHECK_EQUAL(mesh.changes(), 0U);

    // adding a part counts
    mesh.addFace(quad(0.0F), glm::vec3(0.0F, 0.0F, 1.0F));
    const std::uint64_t built = mesh.changes();
    BOOST_CHECK_GT(built, 0U);

    // a part changed in place is counted by touch(), and a restore counts
    const v3d::brep::BRep::Snapshot before = mesh.snapshot();
    mesh.vertex(0)->point(glm::vec3(5.0F));
    mesh.touch();
    BOOST_CHECK_EQUAL(mesh.changes(), built + 1);
    mesh.restore(before);
    BOOST_CHECK_EQUAL(mesh.changes(), built + 2);

    // an operation counts, because it works through the mesh
    BOOST_REQUIRE(v3d::brep::splitEdge(mesh, 0).has_value());
    BOOST_CHECK_GT(mesh.changes(), built + 2);
}

};  // namespace
