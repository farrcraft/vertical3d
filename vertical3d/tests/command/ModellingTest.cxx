/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/Modelling.h>
#include <vertical3d/src/command/TopologyCommand.h>
#include <vertical3d/src/command/VertexCommand.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>

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
 * How many components of a mesh item are selected, across all three kinds.
 **/
std::size_t selectedComponents(const v3d::editor::Scene& scene, const v3d::editor::Mesh& item) {
    std::size_t count = 0;
    for (const v3d::editor::Component kind : { v3d::editor::Component::Vertex, v3d::editor::Component::Edge,
        v3d::editor::Component::Face }) {
        count += scene.components().of(item.id(), kind).size();
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
void undoRedo(v3d::editor::TopologyCommand* command, const v3d::editor::Scene& scene,
    const boost::shared_ptr<v3d::editor::Mesh>& item, const v3d::brep::BRep::Snapshot& before) {
    const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();
    const v3d::brep::BRep::Snapshot after = mesh->snapshot();
    command->undo();
    BOOST_CHECK(same(mesh->snapshot(), before));
    BOOST_CHECK_EQUAL(selectedComponents(scene, *item), 0U);
    BOOST_CHECK(item->selected());
    sound(*mesh);
    command->redo();
    BOOST_CHECK(same(mesh->snapshot(), after));
    BOOST_CHECK_EQUAL(selectedComponents(scene, *item), 0U);
    sound(*mesh);
}

BOOST_AUTO_TEST_CASE(modelling_split_edges_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);

    // nothing selected is nothing to split
    BOOST_CHECK(!v3d::editor::splitEdges(scene));

    scene->components().select(cube->id(), v3d::editor::Component::Edge, 0, true);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitEdges(scene);
    BOOST_REQUIRE(command);
    BOOST_CHECK_EQUAL(command->name(), "split edge");
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(cube->geometry()->vertexCount(), 9U);

    // the two parts of the split edge are selected, so the split can be repeated
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *cube), 2U);
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Edge, 0));
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Edge, cube->geometry()->edge(0)->next()));

    undoRedo(command.get(), *scene, cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_split_boundary_edges_test) {
    // the plane's edges are all on its boundary, and two meshes split in one command
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> plane = boost::make_shared<v3d::editor::Mesh>(v3d::brep::plane());
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    plane->selected(true);
    cube->selected(true);
    scene->add(plane);
    scene->add(cube);
    scene->components().select(plane->id(), v3d::editor::Component::Edge, 1, true);
    scene->components().select(cube->id(), v3d::editor::Component::Edge, 3, true);
    const v3d::brep::BRep::Snapshot planeBefore = plane->geometry()->snapshot();
    const v3d::brep::BRep::Snapshot cubeBefore = cube->geometry()->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitEdges(scene);
    BOOST_REQUIRE(command);
    sound(*plane->geometry());
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(plane->geometry()->edgeCount(), 5U);
    BOOST_CHECK_EQUAL(cube->geometry()->edgeCount(), 26U);

    command->undo();
    BOOST_CHECK(same(plane->geometry()->snapshot(), planeBefore));
    BOOST_CHECK(same(cube->geometry()->snapshot(), cubeBefore));
}

BOOST_AUTO_TEST_CASE(modelling_split_face_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);
    // two opposite corners of the front face
    const v3d::brep::Index first = at(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F));
    const v3d::brep::Index second = at(*cube->geometry(), glm::vec3(0.5F, 0.5F, -0.5F));
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, first, true);
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, second, true);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();

    std::string problem;
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitFace(scene, &problem);
    BOOST_REQUIRE_MESSAGE(command, problem);
    BOOST_CHECK_EQUAL(command->name(), "split face");
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(cube->geometry()->faceCount(), 7U);
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Vertex, first));
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Vertex, second));

    undoRedo(command.get(), *scene, cube, before);
}

/**
 * Two items showing one mesh: an operation runs on it once, with the first item's selection,
 * and the other item's selection, which the renumbering could have moved, is cleared.
 **/
BOOST_AUTO_TEST_CASE(modelling_shared_mesh_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> shared = v3d::brep::cube();
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(shared);
    const boost::shared_ptr<v3d::editor::Mesh> second = boost::make_shared<v3d::editor::Mesh>(shared);
    first->selected(true);
    second->selected(true);
    scene->add(first);
    scene->add(second);
    scene->components().select(first->id(), v3d::editor::Component::Edge, 0, true);
    scene->components().select(second->id(), v3d::editor::Component::Edge, 0, true);
    scene->components().select(second->id(), v3d::editor::Component::Vertex, 3, true);

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::splitEdges(scene);
    BOOST_REQUIRE(command);
    // one split, not two, though both items have the edge selected
    BOOST_CHECK_EQUAL(shared->vertexCount(), 9U);
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *first), 2U);
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *second), 0U);

    // undo clears the selection of every item showing the mesh
    command->undo();
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *first), 0U);
    BOOST_CHECK_EQUAL(shared->vertexCount(), 8U);
}

BOOST_AUTO_TEST_CASE(modelling_split_face_refused_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const boost::shared_ptr<v3d::editor::Mesh> other = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    other->selected(true);
    scene->add(cube);
    scene->add(other);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();
    std::string problem;

    // one vertex is not two
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F)), true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "it needs exactly two vertices selected, and 1 are");

    // corners next to each other are already joined on every face they share
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(0.5F, -0.5F, -0.5F)), true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the two vertices are next to each other on every face they share");

    // opposite corners of the cube share no face
    scene->components().clear(cube->id());
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F)), true);
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(0.5F, 0.5F, 0.5F)), true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the two vertices are not corners of one face");

    // and a vertex in each of two meshes is not a pair in one
    scene->components().clear(cube->id());
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, 0, true);
    scene->components().select(other->id(), v3d::editor::Component::Vertex, 2, true);
    BOOST_CHECK(!v3d::editor::splitFace(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the two vertices are in different meshes");

    BOOST_CHECK(same(cube->geometry()->snapshot(), before));
}

BOOST_AUTO_TEST_CASE(modelling_history_keeps_indices_test) {
    // a vertex move recorded before a split still undoes correctly after the split is undone,
    // because the split's undo puts every index back where the move's record expects it
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);
    const v3d::brep::BRep::Snapshot original = cube->geometry()->snapshot();
    v3d::editor::CommandStack history;

    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(0.5F, 0.5F, -0.5F)), true);
    v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Vertex, scene->components(), scene->evaluate());
    const v3d::editor::VertexCommand::Shape shape = v3d::editor::VertexCommand::Shape::of(target.parts()[0]);
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(0.25F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    history.push(boost::make_shared<v3d::editor::VertexCommand>(std::vector<v3d::editor::VertexCommand::Change>{
        {.part = target.parts()[0], .before = shape, .after = v3d::editor::VertexCommand::Shape::of(target.parts()[0])}},
        "translate"));

    scene->components().clear(cube->id());
    scene->components().select(cube->id(), v3d::editor::Component::Edge, 0, true);
    history.push(v3d::editor::splitEdges(scene));

    history.undo();
    history.undo();
    BOOST_CHECK(same(cube->geometry()->snapshot(), original));
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
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);

    // nothing selected is nothing to extrude
    BOOST_CHECK(!v3d::editor::extrudeFaces(scene));

    const v3d::brep::Index top = facing(*cube->geometry(), glm::vec3(0.0F, 1.0F, 0.0F));
    BOOST_REQUIRE(top != v3d::brep::INVALID_ID);
    scene->components().select(cube->id(), v3d::editor::Component::Face, top, true);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::extrudeFaces(scene);
    BOOST_REQUIRE(command);
    BOOST_CHECK_EQUAL(command->name(), "extrude face");
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(cube->geometry()->faceCount(), 10U);

    // the face stays selected, and the ring is not
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Face, top));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *cube), 1U);
    undoRedo(command.get(), *scene, cube, before);

    // pulled out by a manipulator, the ring's walls get normals that face out of the cube
    scene->components().select(cube->id(), v3d::editor::Component::Face, top, true);
    v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face, scene->components(), scene->evaluate());
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(0.0F, 0.5F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    sound(*cube->geometry());
    for (v3d::brep::Index quad = 6; quad < cube->geometry()->faceCount(); quad++) {
        BOOST_CHECK_SMALL(glm::length(cube->geometry()->face(quad)->normal()) - 1.0F, 0.0001F);
        BOOST_CHECK_SMALL(cube->geometry()->face(quad)->normal().y, 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(modelling_extrude_region_test) {
    // the top and the right share an edge, so they extrude as one region with a ring of six
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);
    const v3d::brep::Index top = facing(*cube->geometry(), glm::vec3(0.0F, 1.0F, 0.0F));
    const v3d::brep::Index right = facing(*cube->geometry(), glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(top != v3d::brep::INVALID_ID && right != v3d::brep::INVALID_ID);
    scene->components().select(cube->id(), v3d::editor::Component::Face, top, true);
    scene->components().select(cube->id(), v3d::editor::Component::Face, right, true);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::extrudeFaces(scene);
    BOOST_REQUIRE(command);
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(cube->geometry()->faceCount(), 12U);
    BOOST_CHECK_EQUAL(cube->geometry()->vertexCount(), 14U);
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *cube), 2U);
    undoRedo(command.get(), *scene, cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_extrude_open_boundary_test) {
    // a wall of the cylinder has its top and bottom on the open rims
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cylinder = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cylinder());
    cylinder->selected(true);
    scene->add(cylinder);
    scene->components().select(cylinder->id(), v3d::editor::Component::Face, 0, true);
    const std::size_t faces = cylinder->geometry()->faceCount();
    const v3d::brep::BRep::Snapshot before = cylinder->geometry()->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::extrudeFaces(scene);
    BOOST_REQUIRE(command);
    sound(*cylinder->geometry());
    BOOST_CHECK_EQUAL(cylinder->geometry()->faceCount(), faces + 4);
    undoRedo(command.get(), *scene, cylinder, before);
}

BOOST_AUTO_TEST_CASE(modelling_inset_faces_test) {
    // two faces that share an edge are each inset on their own
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);
    BOOST_CHECK(!v3d::editor::insetFaces(scene));

    const v3d::brep::Index top = facing(*cube->geometry(), glm::vec3(0.0F, 1.0F, 0.0F));
    const v3d::brep::Index right = facing(*cube->geometry(), glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_REQUIRE(top != v3d::brep::INVALID_ID && right != v3d::brep::INVALID_ID);
    scene->components().select(cube->id(), v3d::editor::Component::Face, top, true);
    scene->components().select(cube->id(), v3d::editor::Component::Face, right, true);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();

    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::insetFaces(scene);
    BOOST_REQUIRE(command);
    BOOST_CHECK_EQUAL(command->name(), "inset face");
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(cube->geometry()->faceCount(), 14U);
    BOOST_CHECK_EQUAL(cube->geometry()->vertexCount(), 16U);
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Face, top));
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Face, right));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *cube), 2U);

    // the top's corners are a quarter of the way in from the cube's, whose top corners are half
    // a unit from its centre along both x and z
    for (const v3d::brep::Index edge : v3d::brep::faceLoop(*cube->geometry(), top)) {
        const glm::vec3 corner = cube->geometry()->vertex(cube->geometry()->edge(edge)->vertex())->point();
        BOOST_CHECK_SMALL(std::abs(corner.x) - (0.5F * (1.0F - v3d::editor::insetFraction)), 0.0001F);
        BOOST_CHECK_SMALL(std::abs(corner.z) - (0.5F * (1.0F - v3d::editor::insetFraction)), 0.0001F);
    }
    undoRedo(command.get(), *scene, cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_merge_vertices_test) {
    // the two ends of one of the cube's edges become one at its middle
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F)), true);
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(0.5F, -0.5F, -0.5F)), true);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();

    std::string problem;
    const boost::shared_ptr<v3d::editor::TopologyCommand> command = v3d::editor::mergeVertices(scene, &problem);
    BOOST_REQUIRE_MESSAGE(command, problem);
    BOOST_CHECK_EQUAL(command->name(), "merge vertices");
    sound(*cube->geometry());
    BOOST_CHECK_EQUAL(cube->geometry()->vertexCount(), 7U);
    BOOST_CHECK_EQUAL(cube->geometry()->faceCount(), 6U);

    // the merged vertex stays selected, so it is the only component that is
    const v3d::brep::Index merged = at(*cube->geometry(), glm::vec3(0.0F, -0.5F, -0.5F));
    BOOST_REQUIRE(merged != v3d::brep::INVALID_ID);
    BOOST_CHECK(scene->components().selected(cube->id(), v3d::editor::Component::Vertex, merged));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, *cube), 1U);

    undoRedo(command.get(), *scene, cube, before);
}

BOOST_AUTO_TEST_CASE(modelling_merge_vertices_refused_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const boost::shared_ptr<v3d::editor::Mesh> other = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    other->selected(true);
    scene->add(cube);
    scene->add(other);
    const v3d::brep::BRep::Snapshot before = cube->geometry()->snapshot();
    std::string problem;

    // one vertex is not enough
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F)), true);
    BOOST_CHECK(!v3d::editor::mergeVertices(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "it needs at least two vertices selected, and 1 are");

    // opposite corners of one face would fold the face onto itself
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, at(*cube->geometry(), glm::vec3(0.5F, 0.5F, -0.5F)), true);
    BOOST_CHECK(!v3d::editor::mergeVertices(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "it would leave an edge shared by more than two faces, or a face that meets itself");
    BOOST_CHECK(same(cube->geometry()->snapshot(), before));

    // and a vertex in each of two meshes cannot be merged
    scene->components().clear(cube->id());
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, 0, true);
    scene->components().select(other->id(), v3d::editor::Component::Vertex, 1, true);
    BOOST_CHECK(!v3d::editor::mergeVertices(scene, &problem));
    BOOST_CHECK_EQUAL(problem, "the vertices are in different meshes");
}

BOOST_AUTO_TEST_CASE(modelling_merge_history_keeps_indices_test) {
    // a merge removes a vertex and renumbers those after it. A move of the last vertex, recorded
    // before the merge, still undoes correctly once the merge is undone
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);
    const v3d::brep::BRep::Snapshot original = cube->geometry()->snapshot();
    v3d::editor::CommandStack history;

    scene->components().select(cube->id(), v3d::editor::Component::Vertex, static_cast<v3d::brep::Index>(cube->geometry()->vertexCount() - 1), true);
    v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Vertex, scene->components(), scene->evaluate());
    const v3d::editor::VertexCommand::Shape shape = v3d::editor::VertexCommand::Shape::of(target.parts()[0]);
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(0.25F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    history.push(boost::make_shared<v3d::editor::VertexCommand>(std::vector<v3d::editor::VertexCommand::Change>{
        {.part = target.parts()[0], .before = shape, .after = v3d::editor::VertexCommand::Shape::of(target.parts()[0])}},
        "translate"));

    // the two vertices merged are the ends of the second edge of the first face
    scene->components().clear(cube->id());
    const v3d::brep::Index first = cube->geometry()->edge(cube->geometry()->face(0)->edge())->vertex();
    const v3d::brep::Index second = cube->geometry()->edge(cube->geometry()->edge(cube->geometry()->face(0)->edge())->next())->vertex();
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, first, true);
    scene->components().select(cube->id(), v3d::editor::Component::Vertex, second, true);
    std::string problem;
    const boost::shared_ptr<v3d::editor::TopologyCommand> merge = v3d::editor::mergeVertices(scene, &problem);
    BOOST_REQUIRE_MESSAGE(merge, problem);
    history.push(merge);
    BOOST_CHECK_EQUAL(cube->geometry()->vertexCount(), 7U);

    history.undo();
    history.undo();
    BOOST_CHECK(same(cube->geometry()->snapshot(), original));
}

};  // namespace
