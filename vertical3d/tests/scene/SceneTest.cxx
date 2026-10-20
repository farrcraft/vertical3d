/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/SceneVisitor.h>

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * Records what it was handed, in the order it was handed it.
 **/
class CountingVisitor final : public v3d::editor::SceneVisitor {
 public:
    void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) override {
        visited.push_back(mesh->id());
    }

    std::vector<unsigned int> visited;
};

BOOST_AUTO_TEST_CASE(scene_empty_test) {
    v3d::editor::Scene scene;

    BOOST_CHECK_EQUAL(scene.count(), 0U);
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.mesh(1)), false);
    BOOST_CHECK_EQUAL(scene.remove(1), false);

    CountingVisitor visitor;
    scene.accept(&visitor);
    BOOST_CHECK_EQUAL(visitor.visited.size(), 0U);

    // an empty scene is an unopened document, not an error
    scene.accept(nullptr);
}

BOOST_AUTO_TEST_CASE(scene_add_test) {
    v3d::editor::Scene scene;

    const boost::shared_ptr<v3d::brep::BRep> first = boost::make_shared<v3d::brep::BRep>();
    const boost::shared_ptr<v3d::brep::BRep> second = boost::make_shared<v3d::brep::BRep>();

    const unsigned int firstID = scene.add(first);
    const unsigned int secondID = scene.add(second);

    BOOST_CHECK_EQUAL(scene.count(), 2U);
    BOOST_CHECK_EQUAL(firstID, first->id());
    BOOST_CHECK(firstID != secondID);

    // a mesh is found by the id its node base carries, not by where it sits
    BOOST_CHECK_EQUAL(scene.mesh(secondID).get(), second.get());
    BOOST_CHECK_EQUAL(scene.remove(firstID), true);
    BOOST_CHECK_EQUAL(scene.count(), 1U);
    BOOST_CHECK_EQUAL(scene.mesh(secondID).get(), second.get());
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.mesh(firstID)), false);

    // an empty pointer is not a mesh, and adding one leaves the scene alone
    BOOST_CHECK_EQUAL(scene.add(boost::shared_ptr<v3d::brep::BRep>()), 0U);
    BOOST_CHECK_EQUAL(scene.count(), 1U);

    scene.clear();
    BOOST_CHECK_EQUAL(scene.count(), 0U);
}

BOOST_AUTO_TEST_CASE(scene_visit_order_test) {
    v3d::editor::Scene scene;

    const boost::shared_ptr<v3d::brep::BRep> first = boost::make_shared<v3d::brep::BRep>();
    const boost::shared_ptr<v3d::brep::BRep> second = boost::make_shared<v3d::brep::BRep>();
    scene.add(first);
    scene.add(second);

    CountingVisitor visitor;
    scene.accept(&visitor);

    BOOST_REQUIRE_EQUAL(visitor.visited.size(), 2U);
    BOOST_CHECK_EQUAL(visitor.visited[0], first->id());
    BOOST_CHECK_EQUAL(visitor.visited[1], second->id());
}

BOOST_AUTO_TEST_CASE(scene_deselect_test) {
    v3d::editor::Scene scene;

    const boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    std::vector<glm::vec3> quad;
    quad.push_back(glm::vec3(0.0F, 0.0F, 0.0F));
    quad.push_back(glm::vec3(1.0F, 0.0F, 0.0F));
    quad.push_back(glm::vec3(1.0F, 1.0F, 0.0F));
    quad.push_back(glm::vec3(0.0F, 1.0F, 0.0F));
    mesh->addFace(quad, glm::vec3(0.0F, 0.0F, 1.0F));
    mesh->selected(true);
    mesh->edge(0)->selected(true);
    scene.add(mesh);

    scene.deselect();

    // a scene wide deselect clears the objects and their components both
    BOOST_CHECK_EQUAL(mesh->selected(), false);
    BOOST_CHECK_EQUAL(mesh->edge(0)->selected(), false);
}

BOOST_AUTO_TEST_CASE(scene_selection_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> first = boost::make_shared<v3d::brep::BRep>();
    const boost::shared_ptr<v3d::brep::BRep> second = boost::make_shared<v3d::brep::BRep>();
    const boost::shared_ptr<v3d::brep::BRep> third = boost::make_shared<v3d::brep::BRep>();
    scene.add(first);
    scene.add(second);
    scene.add(third);
    BOOST_CHECK(scene.selection().empty());

    // the set comes back in insertion order, whatever order the flags were set in
    third->selected(true);
    first->selected(true);
    const v3d::editor::Meshes selection = scene.selection();
    BOOST_REQUIRE_EQUAL(selection.size(), 2U);
    BOOST_CHECK(selection[0] == first);
    BOOST_CHECK(selection[1] == third);
}

};  // namespace
