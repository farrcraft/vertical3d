/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/TopologyCommand.h>
#include <vertical3d/src/command/VertexCommand.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Modelling.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectMask.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * Whether two snapshots hold the same mesh, index for index.
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
 * How many components of a mesh are selected, across all three kinds.
 **/
std::size_t selectedComponents(const v3d::brep::BRep& mesh) {
    std::size_t count = 0;
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        count += mesh.vertex(id)->selected() ? 1 : 0;
    }
    for (v3d::brep::Index id = 0; id < mesh.edgeCount(); id++) {
        count += mesh.edge(id)->selected() ? 1 : 0;
    }
    for (v3d::brep::Index id = 0; id < mesh.faceCount(); id++) {
        count += mesh.face(id)->selected() ? 1 : 0;
    }
    return count;
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

/**
 * Whether the mesh passes the strict check, reporting why when it does not.
 **/
void sound(const v3d::brep::BRep& mesh) {
    std::string problem;
    BOOST_CHECK_MESSAGE(mesh.validateStrict(&problem), problem);
}

/**
 * Undo and redo a topology change, and check that each puts the mesh back index for index,
 * clears its components, and keeps the object selected.
 **/
void undoRedo(v3d::editor::TopologyCommand* command, const boost::shared_ptr<v3d::brep::BRep>& mesh,
    const v3d::brep::BRep::Snapshot& before) {
    const v3d::brep::BRep::Snapshot after = mesh->snapshot();
    command->undo();
    BOOST_CHECK(same(mesh->snapshot(), before));
    BOOST_CHECK_EQUAL(selectedComponents(*mesh), 0U);
    BOOST_CHECK(mesh->selected());
    sound(*mesh);
    command->redo();
    BOOST_CHECK(same(mesh->snapshot(), after));
    BOOST_CHECK_EQUAL(selectedComponents(*mesh), 0U);
    sound(*mesh);
}

BOOST_AUTO_TEST_CASE(modelling_split_edges_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);

    // nothing selected is nothing to split
    BOOST_CHECK(!v3d::editor::splitEdges(scene));

    cube->edge(0)->selected(true);
    const v3d::brep::BRep::Snapshot before = cube->snapshot();
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitEdges(scene);
    BOOST_REQUIRE(command);
    BOOST_CHECK_EQUAL(command->name(), "split edge");
    sound(*cube);
    BOOST_CHECK_EQUAL(cube->vertexCount(), 9U);

    // the two parts of the split edge are selected, so the split can be repeated
    BOOST_CHECK_EQUAL(selectedComponents(*cube), 2U);
    BOOST_CHECK(cube->edge(0)->selected());
    BOOST_CHECK(cube->edge(cube->edge(0)->next())->selected());

    undoRedo(command.get(), cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_split_boundary_edges_test) {
    // the plane's edges are all on its boundary, and two meshes split in one command
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> plane = v3d::editor::create_poly_plane();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    plane->selected(true);
    cube->selected(true);
    scene.add(plane);
    scene.add(cube);
    plane->edge(1)->selected(true);
    cube->edge(3)->selected(true);
    const v3d::brep::BRep::Snapshot planeBefore = plane->snapshot();
    const v3d::brep::BRep::Snapshot cubeBefore = cube->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitEdges(scene);
    BOOST_REQUIRE(command);
    sound(*plane);
    sound(*cube);
    BOOST_CHECK_EQUAL(plane->edgeCount(), 5U);
    BOOST_CHECK_EQUAL(cube->edgeCount(), 26U);

    command->undo();
    BOOST_CHECK(same(plane->snapshot(), planeBefore));
    BOOST_CHECK(same(cube->snapshot(), cubeBefore));
}

BOOST_AUTO_TEST_CASE(modelling_split_face_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);
    // two opposite corners of the front face
    const v3d::brep::Index first = at(*cube, glm::vec3(-0.5F, -0.5F, -0.5F));
    const v3d::brep::Index second = at(*cube, glm::vec3(0.5F, 0.5F, -0.5F));
    cube->vertex(first)->selected(true);
    cube->vertex(second)->selected(true);
    const v3d::brep::BRep::Snapshot before = cube->snapshot();

    std::string problem;
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitFace(scene, &problem);
    BOOST_REQUIRE_MESSAGE(command, problem);
    BOOST_CHECK_EQUAL(command->name(), "split face");
    sound(*cube);
    BOOST_CHECK_EQUAL(cube->faceCount(), 7U);
    BOOST_CHECK(cube->vertex(first)->selected());
    BOOST_CHECK(cube->vertex(second)->selected());

    undoRedo(command.get(), cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_split_face_refused_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const boost::shared_ptr<v3d::brep::BRep> other = v3d::editor::create_poly_cube();
    cube->selected(true);
    other->selected(true);
    scene.add(cube);
    scene.add(other);
    const v3d::brep::BRep::Snapshot before = cube->snapshot();
    std::string problem;

    // one vertex is not two
    cube->vertex(at(*cube, glm::vec3(-0.5F, -0.5F, -0.5F)))->selected(true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "it needs exactly two vertices selected, and 1 are");

    // corners next to each other are already joined on every face they share
    cube->vertex(at(*cube, glm::vec3(0.5F, -0.5F, -0.5F)))->selected(true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the two vertices are next to each other on every face they share");

    // opposite corners of the cube share no face
    cube->deselectComponents();
    cube->vertex(at(*cube, glm::vec3(-0.5F, -0.5F, -0.5F)))->selected(true);
    cube->vertex(at(*cube, glm::vec3(0.5F, 0.5F, 0.5F)))->selected(true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the two vertices are not corners of one face");

    // and a vertex in each of two meshes is not a pair in one
    cube->deselectComponents();
    cube->vertex(0)->selected(true);
    other->vertex(2)->selected(true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the two vertices are in different meshes");

    BOOST_CHECK(same(cube->snapshot(), before));
}

BOOST_AUTO_TEST_CASE(modelling_history_keeps_indices_test) {
    // a vertex move recorded before a split still undoes correctly after the split is undone,
    // because the split's undo puts every index back where the move's record expects it
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);
    const v3d::brep::BRep::Snapshot original = cube->snapshot();
    v3d::editor::CommandStack history;

    cube->vertex(at(*cube, glm::vec3(0.5F, 0.5F, -0.5F)))->selected(true);
    const v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Vertex);
    const v3d::editor::VertexCommand::Shape shape = v3d::editor::VertexCommand::Shape::of(target.parts()[0]);
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(0.25F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    history.push(boost::make_shared<v3d::editor::VertexCommand>(std::vector<v3d::editor::VertexCommand::Change>{
        {.part = target.parts()[0], .before = shape, .after = v3d::editor::VertexCommand::Shape::of(target.parts()[0])}},
        "translate"));

    cube->deselectComponents();
    cube->edge(0)->selected(true);
    history.push(v3d::editor::splitEdges(scene));

    history.undo();
    history.undo();
    BOOST_CHECK(same(cube->snapshot(), original));
}

/**
 * The index of a mesh's face whose normal points one way.
 **/
v3d::brep::Index facing(const v3d::brep::BRep& mesh, const glm::vec3& normal) {
    for (v3d::brep::Index id = 0; id < mesh.faceCount(); id++) {
        if (glm::distance(mesh.face(id)->normal(), normal) < 0.0001F) {
            return id;
        }
    }
    return v3d::brep::INVALID_ID;
}

BOOST_AUTO_TEST_CASE(modelling_extrude_faces_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);

    // nothing selected is nothing to extrude
    BOOST_CHECK(!v3d::editor::extrudeFaces(scene));

    const v3d::brep::Index top = facing(*cube, glm::vec3(0.0F, 1.0F, 0.0F));
    BOOST_REQUIRE(top != v3d::brep::INVALID_ID);
    cube->face(top)->selected(true);
    const v3d::brep::BRep::Snapshot before = cube->snapshot();
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::extrudeFaces(scene);
    BOOST_REQUIRE(command);
    BOOST_CHECK_EQUAL(command->name(), "extrude face");
    sound(*cube);
    BOOST_CHECK_EQUAL(cube->faceCount(), 10U);

    // the face stays selected, and the ring is not
    BOOST_CHECK(cube->face(top)->selected());
    BOOST_CHECK_EQUAL(selectedComponents(*cube), 1U);
    undoRedo(command.get(), cube, before);

    // pulled out by a manipulator, the ring's walls get normals that face out of the cube
    cube->face(top)->selected(true);
    const v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face);
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(0.0F, 0.5F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    sound(*cube);
    for (v3d::brep::Index quad = 6; quad < cube->faceCount(); quad++) {
        BOOST_CHECK_SMALL(glm::length(cube->face(quad)->normal()) - 1.0F, 0.0001F);
        BOOST_CHECK_SMALL(cube->face(quad)->normal().y, 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(modelling_extrude_region_test) {
    // the top and the right share an edge, so they extrude as one region with a ring of six
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);
    const v3d::brep::Index top = facing(*cube, glm::vec3(0.0F, 1.0F, 0.0F));
    const v3d::brep::Index right = facing(*cube, glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(top != v3d::brep::INVALID_ID && right != v3d::brep::INVALID_ID);
    cube->face(top)->selected(true);
    cube->face(right)->selected(true);
    const v3d::brep::BRep::Snapshot before = cube->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::extrudeFaces(scene);
    BOOST_REQUIRE(command);
    sound(*cube);
    BOOST_CHECK_EQUAL(cube->faceCount(), 12U);
    BOOST_CHECK_EQUAL(cube->vertexCount(), 14U);
    BOOST_CHECK_EQUAL(selectedComponents(*cube), 2U);
    undoRedo(command.get(), cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_extrude_open_boundary_test) {
    // a wall of the cylinder has its top and bottom on the open rims
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cylinder = v3d::editor::create_poly_cylinder();
    cylinder->selected(true);
    scene.add(cylinder);
    cylinder->face(0)->selected(true);
    const std::size_t faces = cylinder->faceCount();
    const v3d::brep::BRep::Snapshot before = cylinder->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::extrudeFaces(scene);
    BOOST_REQUIRE(command);
    sound(*cylinder);
    BOOST_CHECK_EQUAL(cylinder->faceCount(), faces + 4);
    undoRedo(command.get(), cylinder, before);
}

BOOST_AUTO_TEST_CASE(modelling_inset_faces_test) {
    // two faces that share an edge are each inset on their own
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);
    BOOST_CHECK(!v3d::editor::insetFaces(scene));

    const v3d::brep::Index top = facing(*cube, glm::vec3(0.0F, 1.0F, 0.0F));
    const v3d::brep::Index right = facing(*cube, glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(top != v3d::brep::INVALID_ID && right != v3d::brep::INVALID_ID);
    cube->face(top)->selected(true);
    cube->face(right)->selected(true);
    const v3d::brep::BRep::Snapshot before = cube->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::insetFaces(scene);
    BOOST_REQUIRE(command);
    BOOST_CHECK_EQUAL(command->name(), "inset face");
    sound(*cube);
    BOOST_CHECK_EQUAL(cube->faceCount(), 14U);
    BOOST_CHECK_EQUAL(cube->vertexCount(), 16U);
    BOOST_CHECK(cube->face(top)->selected());
    BOOST_CHECK(cube->face(right)->selected());
    BOOST_CHECK_EQUAL(selectedComponents(*cube), 2U);

    // the top's corners are a quarter of the way in from the cube's, whose top corners are half
    // a unit from its centre along both x and z
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(*cube, top)) {
        const glm::vec3 corner = cube->vertex(cube->edge(edge)->vertex())->point();
        BOOST_CHECK_SMALL(std::abs(corner.x) - (0.5F * (1.0F - v3d::editor::insetFraction)), 0.0001F);
        BOOST_CHECK_SMALL(std::abs(corner.z) - (0.5F * (1.0F - v3d::editor::insetFraction)), 0.0001F);
    }
    undoRedo(command.get(), cube, before);
}

};  // namespace
