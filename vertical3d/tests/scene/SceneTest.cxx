/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/SceneVisitor.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * Records what it was handed, in the order it was handed it, and through which visit().
 **/
class CountingVisitor final : public v3d::editor::SceneVisitor {
 public:
    void visit(const boost::shared_ptr<v3d::editor::Mesh>& mesh) override {
        visited.push_back(mesh->id());
        kinds += "m";
    }

    void visit(const boost::shared_ptr<v3d::editor::Camera>& camera) override {
        visited.push_back(camera->id());
        kinds += "c";
    }

    void visit(const boost::shared_ptr<v3d::editor::Light>& light) override {
        visited.push_back(light->id());
        kinds += "l";
    }

    std::vector<unsigned int> visited;
    std::string kinds;
};

boost::shared_ptr<v3d::editor::Mesh> emptyMesh() {
    return boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
}

BOOST_AUTO_TEST_CASE(scene_empty_test) {
    v3d::editor::Scene scene;

    BOOST_CHECK_EQUAL(scene.count(), 0U);
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.mesh(1)), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.item(1)), false);
    BOOST_CHECK_EQUAL(scene.remove(1), false);

    CountingVisitor visitor;
    scene.accept(&visitor);
    BOOST_CHECK_EQUAL(visitor.visited.size(), 0U);

    // an empty scene is an unopened document, not an error
    scene.accept(nullptr);
}

BOOST_AUTO_TEST_CASE(scene_add_test) {
    v3d::editor::Scene scene;

    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> second = emptyMesh();

    const unsigned int firstID = scene.add(first);
    const unsigned int secondID = scene.add(second);

    BOOST_CHECK_EQUAL(scene.count(), 2U);
    BOOST_CHECK_EQUAL(firstID, first->id());
    BOOST_CHECK(firstID != secondID);

    // an item is found by the id its node base carries, not by where it sits
    BOOST_CHECK_EQUAL(scene.mesh(secondID).get(), second.get());
    BOOST_CHECK_EQUAL(scene.remove(firstID), true);
    BOOST_CHECK_EQUAL(scene.count(), 1U);
    BOOST_CHECK_EQUAL(scene.mesh(secondID).get(), second.get());
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.mesh(firstID)), false);

    // an empty pointer is not an item, and adding one leaves the scene alone
    BOOST_CHECK_EQUAL(scene.add(boost::shared_ptr<v3d::editor::Item>()), 0U);
    BOOST_CHECK_EQUAL(scene.count(), 1U);

    scene.clear();
    BOOST_CHECK_EQUAL(scene.count(), 0U);
}

BOOST_AUTO_TEST_CASE(scene_kinds_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    scene.add(camera);
    scene.add(mesh);
    scene.add(light);

    // every kind is held in the one list, and found by its id
    BOOST_CHECK_EQUAL(scene.count(), 3U);
    BOOST_CHECK(scene.item(camera->id()) == camera);
    BOOST_CHECK(scene.item(light->id()) == light);

    // the typed lookup finds only a mesh
    BOOST_CHECK(scene.mesh(mesh->id()) == mesh);
    BOOST_CHECK(!scene.mesh(camera->id()));

    BOOST_CHECK_EQUAL(scene.remove(camera->id()), true);
    BOOST_CHECK(!scene.item(camera->id()));
    BOOST_CHECK_EQUAL(scene.count(), 2U);
}

BOOST_AUTO_TEST_CASE(scene_visit_order_test) {
    v3d::editor::Scene scene;

    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    const boost::shared_ptr<v3d::editor::Mesh> second = emptyMesh();
    scene.add(first);
    scene.add(light);
    scene.add(camera);
    scene.add(second);

    CountingVisitor visitor;
    scene.accept(&visitor);

    // insertion order across the kinds, each through the visit() for its own kind
    BOOST_REQUIRE_EQUAL(visitor.visited.size(), 4U);
    BOOST_CHECK_EQUAL(visitor.visited[0], first->id());
    BOOST_CHECK_EQUAL(visitor.visited[1], light->id());
    BOOST_CHECK_EQUAL(visitor.visited[2], camera->id());
    BOOST_CHECK_EQUAL(visitor.visited[3], second->id());
    BOOST_CHECK_EQUAL(visitor.kinds, "mlcm");
}

BOOST_AUTO_TEST_CASE(scene_deselect_test) {
    v3d::editor::Scene scene;

    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    std::vector<glm::vec3> quad;
    quad.push_back(glm::vec3(0.0F, 0.0F, 0.0F));
    quad.push_back(glm::vec3(1.0F, 0.0F, 0.0F));
    quad.push_back(glm::vec3(1.0F, 1.0F, 0.0F));
    quad.push_back(glm::vec3(0.0F, 1.0F, 0.0F));
    mesh->geometry()->addFace(quad, glm::vec3(0.0F, 0.0F, 1.0F));
    mesh->selected(true);
    mesh->geometry()->edge(0)->selected(true);
    scene.add(mesh);
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    light->selected(true);
    scene.add(light);

    scene.deselectComponents();

    // a component deselect leaves the objects selected
    BOOST_CHECK_EQUAL(mesh->geometry()->edge(0)->selected(), false);
    BOOST_CHECK_EQUAL(mesh->selected(), true);

    mesh->geometry()->edge(0)->selected(true);
    scene.deselect();

    // a scene wide deselect clears the items of every kind, and the components too
    BOOST_CHECK_EQUAL(mesh->selected(), false);
    BOOST_CHECK_EQUAL(light->selected(), false);
    BOOST_CHECK_EQUAL(mesh->geometry()->edge(0)->selected(), false);
}

BOOST_AUTO_TEST_CASE(scene_selection_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    const boost::shared_ptr<v3d::editor::Mesh> second = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> third = emptyMesh();
    scene.add(first);
    scene.add(camera);
    scene.add(second);
    scene.add(third);
    BOOST_CHECK(scene.selection().empty());

    // the set comes back in insertion order, whatever order the flags were set in
    third->selected(true);
    camera->selected(true);
    first->selected(true);
    const v3d::editor::Items selection = scene.selection();
    BOOST_REQUIRE_EQUAL(selection.size(), 3U);
    BOOST_CHECK(selection[0] == first);
    BOOST_CHECK(selection[1] == camera);
    BOOST_CHECK(selection[2] == third);

    // and of one kind, the selected items of that kind only
    const v3d::editor::Meshes meshes = scene.selected<v3d::editor::Mesh>();
    BOOST_REQUIRE_EQUAL(meshes.size(), 2U);
    BOOST_CHECK(meshes[0] == first);
    BOOST_CHECK(meshes[1] == third);
    BOOST_CHECK_EQUAL(scene.selected<v3d::editor::Camera>().size(), 1U);
    BOOST_CHECK(scene.selected<v3d::editor::Light>().empty());
}

};  // namespace
