/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../../src/scene/Scene.h"

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

};  // namespace

BOOST_AUTO_TEST_CASE(scene_empty_test) {
    v3d::editor::Scene scene;

    BOOST_CHECK_EQUAL(scene.count(), 0u);
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.mesh(1)), false);
    BOOST_CHECK_EQUAL(scene.remove(1), false);

    CountingVisitor visitor;
    scene.accept(&visitor);
    BOOST_CHECK_EQUAL(visitor.visited.size(), 0u);

    // an empty scene is what an unopened document is, not an error
    scene.accept(nullptr);
}

BOOST_AUTO_TEST_CASE(scene_add_test) {
    v3d::editor::Scene scene;

    boost::shared_ptr<v3d::brep::BRep> first = boost::make_shared<v3d::brep::BRep>();
    boost::shared_ptr<v3d::brep::BRep> second = boost::make_shared<v3d::brep::BRep>();

    const unsigned int firstID = scene.add(first);
    const unsigned int secondID = scene.add(second);

    BOOST_CHECK_EQUAL(scene.count(), 2u);
    BOOST_CHECK_EQUAL(firstID, first->id());
    BOOST_CHECK(firstID != secondID);

    // a mesh is found by the id its node base carries, not by where it sits
    BOOST_CHECK_EQUAL(scene.mesh(secondID).get(), second.get());
    BOOST_CHECK_EQUAL(scene.remove(firstID), true);
    BOOST_CHECK_EQUAL(scene.count(), 1u);
    BOOST_CHECK_EQUAL(scene.mesh(secondID).get(), second.get());
    BOOST_CHECK_EQUAL(static_cast<bool>(scene.mesh(firstID)), false);

    // an empty pointer is not a mesh, and adding one leaves the scene alone
    BOOST_CHECK_EQUAL(scene.add(boost::shared_ptr<v3d::brep::BRep>()), 0u);
    BOOST_CHECK_EQUAL(scene.count(), 1u);

    scene.clear();
    BOOST_CHECK_EQUAL(scene.count(), 0u);
}

BOOST_AUTO_TEST_CASE(scene_visit_order_test) {
    v3d::editor::Scene scene;

    boost::shared_ptr<v3d::brep::BRep> first = boost::make_shared<v3d::brep::BRep>();
    boost::shared_ptr<v3d::brep::BRep> second = boost::make_shared<v3d::brep::BRep>();
    scene.add(first);
    scene.add(second);

    CountingVisitor visitor;
    scene.accept(&visitor);

    BOOST_REQUIRE_EQUAL(visitor.visited.size(), 2u);
    BOOST_CHECK_EQUAL(visitor.visited[0], first->id());
    BOOST_CHECK_EQUAL(visitor.visited[1], second->id());
}

BOOST_AUTO_TEST_CASE(scene_deselect_test) {
    v3d::editor::Scene scene;

    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    std::vector<glm::vec3> quad;
    quad.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    quad.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    quad.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
    quad.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    mesh->addFace(quad, glm::vec3(0.0f, 0.0f, 1.0f));
    mesh->selected(true);
    mesh->edge(0)->selected(true);
    scene.add(mesh);

    scene.deselect();

    // a scene wide deselect clears the objects and their components both
    BOOST_CHECK_EQUAL(mesh->selected(), false);
    BOOST_CHECK_EQUAL(mesh->edge(0)->selected(), false);
}
