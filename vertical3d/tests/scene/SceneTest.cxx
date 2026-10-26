/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/animation/Curve.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/SceneVisitor.h>
#include <vertical3d/src/scene/Timeline.h>

#include <cstdint>
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
    void visit(const boost::shared_ptr<v3d::editor::Mesh>& mesh, const v3d::editor::Placed& /*placed*/) override {
        visited.push_back(mesh->id());
        kinds += "m";
    }

    void visit(const boost::shared_ptr<v3d::editor::Camera>& camera, const v3d::editor::Placed& /*placed*/) override {
        visited.push_back(camera->id());
        kinds += "c";
    }

    void visit(const boost::shared_ptr<v3d::editor::Light>& light, const v3d::editor::Placed& /*placed*/) override {
        visited.push_back(light->id());
        kinds += "l";
    }

    void visit(const boost::shared_ptr<v3d::editor::Group>& group, const v3d::editor::Placed& /*placed*/) override {
        visited.push_back(group->id());
        kinds += "g";
    }

    void leave(const boost::shared_ptr<v3d::editor::Item>& item) override {
        left.push_back(item->id());
    }

    std::vector<unsigned int> visited;
    std::vector<unsigned int> left;
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

BOOST_AUTO_TEST_CASE(scene_document_ids_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Camera> second = boost::make_shared<v3d::editor::Camera>();
    BOOST_CHECK_EQUAL(first->documentId(), 0);

    // the scene gives each item the next document id as it is first added
    scene.add(first);
    scene.add(second);
    BOOST_CHECK_EQUAL(first->documentId(), 1);
    BOOST_CHECK_EQUAL(second->documentId(), 2);
    BOOST_CHECK_EQUAL(scene.nextDocumentId(), 3);

    // an item taken out and put back, as an undone creation is redone, keeps its id, and the
    // id is not given to anything else meanwhile
    scene.remove(first->id());
    const boost::shared_ptr<v3d::editor::Light> third = boost::make_shared<v3d::editor::Light>();
    scene.add(third);
    scene.add(first);
    BOOST_CHECK_EQUAL(first->documentId(), 1);
    BOOST_CHECK_EQUAL(third->documentId(), 3);

    // a closed document starts its ids again
    scene.clear();
    BOOST_CHECK_EQUAL(scene.nextDocumentId(), 1);
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

BOOST_AUTO_TEST_CASE(scene_parent_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    scene.add(mesh);
    scene.add(group);
    scene.add(light);

    // nothing has a parent until one is given
    BOOST_CHECK(!scene.parent(mesh->id()));
    BOOST_CHECK_EQUAL(scene.children(0).size(), 3U);

    BOOST_CHECK(scene.parent(mesh->id(), group->id()));
    BOOST_CHECK(scene.parent(group->id(), light->id()));
    BOOST_CHECK(scene.parent(mesh->id()) == group);
    const v3d::editor::Items above = scene.ancestors(mesh->id());
    BOOST_REQUIRE_EQUAL(above.size(), 2U);
    BOOST_CHECK(above[0] == group);
    BOOST_CHECK(above[1] == light);
    BOOST_CHECK(scene.ancestors(light->id()).empty());

    // the items with no parent, and the items beneath one, come in the scene's order
    const v3d::editor::Items roots = scene.children(0);
    BOOST_REQUIRE_EQUAL(roots.size(), 1U);
    BOOST_CHECK(roots[0] == light);
    BOOST_REQUIRE_EQUAL(scene.children(group->id()).size(), 1U);
    BOOST_CHECK(scene.children(group->id())[0] == mesh);

    // an item cannot be its own parent, nor go beneath anything beneath it, and a link to an
    // item the scene does not hold is refused. Each refusal leaves the hierarchy as it was
    BOOST_CHECK(!scene.parent(group->id(), group->id()));
    BOOST_CHECK(!scene.parent(light->id(), mesh->id()));
    BOOST_CHECK(!scene.parent(mesh->id(), boost::make_shared<v3d::editor::Group>()->id()));
    BOOST_CHECK(!scene.parent(boost::make_shared<v3d::editor::Group>()->id(), group->id()));
    BOOST_CHECK(scene.parent(group->id()) == light);
    BOOST_CHECK(!scene.parent(light->id()));

    // a parent of 0 leaves the item with none
    BOOST_CHECK(scene.parent(group->id(), 0));
    BOOST_CHECK(!scene.parent(group->id()));
    BOOST_CHECK_EQUAL(scene.children(0).size(), 2U);

    // a closed document has no hierarchy left
    scene.clear();
    scene.add(mesh);
    scene.add(group);
    BOOST_CHECK(!scene.parent(mesh->id()));
}

BOOST_AUTO_TEST_CASE(scene_remove_beneath_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> top = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Group> middle = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> bottom = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> apart = emptyMesh();
    scene.add(bottom);
    scene.add(top);
    scene.add(apart);
    scene.add(middle);
    scene.parent(middle->id(), top->id());
    scene.parent(bottom->id(), middle->id());

    // removing an item takes everything beneath it, at every depth, and nothing else
    BOOST_CHECK(scene.remove(top->id()));
    BOOST_CHECK_EQUAL(scene.count(), 1U);
    BOOST_CHECK(scene.item(apart->id()) == apart);

    // the links among them go too, so the items come back with no parent
    scene.add(top);
    scene.add(middle);
    BOOST_CHECK(!scene.parent(middle->id()));
    BOOST_CHECK(scene.children(top->id()).empty());

    // removing a child leaves its parent where it is
    scene.parent(middle->id(), top->id());
    BOOST_CHECK(scene.remove(middle->id()));
    BOOST_CHECK(scene.item(top->id()) == top);
    BOOST_CHECK(scene.children(top->id()).empty());
}

BOOST_AUTO_TEST_CASE(scene_visit_beneath_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> child = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> loose = emptyMesh();
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    scene.add(child);
    scene.add(loose);
    scene.add(group);
    scene.add(light);
    scene.parent(child->id(), group->id());
    scene.parent(light->id(), group->id());

    CountingVisitor visitor;
    scene.accept(&visitor);

    // each parent before its children, though the children came first in the scene, and
    // children in the scene's order
    const std::vector<unsigned int> visited{ loose->id(), group->id(), child->id(), light->id() };
    BOOST_CHECK(visitor.visited == visited);
    BOOST_CHECK_EQUAL(visitor.kinds, "mgml");

    // an item is left once everything beneath it has been
    const std::vector<unsigned int> left{ loose->id(), child->id(), light->id(), group->id() };
    BOOST_CHECK(visitor.left == left);
}

/**
 * A curve of two keys, with the values given at frames 1 and 11.
 **/
v3d::animation::Curve between(float first, float second) {
    v3d::animation::Curve curve;
    curve.set(1.0F, first);
    curve.set(11.0F, second);
    return curve;
}

BOOST_AUTO_TEST_CASE(scene_curves_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    scene.add(mesh);
    scene.add(light);

    // a curve is set on a property the item's kind declares, and refused on any other or on
    // an item the scene does not hold
    BOOST_CHECK(scene.curve(mesh->id(), "translation.x", between(0.0F, 10.0F)));
    BOOST_CHECK(!scene.curve(mesh->id(), "emission.intensity", between(0.0F, 1.0F)));
    BOOST_CHECK(!scene.curve(emptyMesh()->id(), "translation.x", between(0.0F, 1.0F)));
    BOOST_CHECK(scene.curve(light->id(), "emission.intensity", between(1.0F, 3.0F)));
    BOOST_CHECK_EQUAL(scene.curves(mesh->id()).size(), 1U);
    BOOST_CHECK(scene.curves(mesh->id()).at("translation.x") == between(0.0F, 10.0F));

    // the keyed frames of a set of items come back rising, each once
    BOOST_CHECK(scene.curve(light->id(), "emission.red", between(0.0F, 1.0F)));
    const std::vector<float> frames = scene.keyedFrames(scene.items());
    BOOST_REQUIRE_EQUAL(frames.size(), 2U);
    BOOST_CHECK_EQUAL(frames[0], 1.0F);
    BOOST_CHECK_EQUAL(frames[1], 11.0F);

    // an empty curve removes one, and removing an item takes its curves
    BOOST_CHECK(scene.curve(mesh->id(), "translation.x", v3d::animation::Curve()));
    BOOST_CHECK(scene.curves(mesh->id()).empty());
    scene.remove(light->id());
    scene.add(light);
    BOOST_CHECK(scene.curves(light->id()).empty());
}

BOOST_AUTO_TEST_CASE(scene_pose_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> still = emptyMesh();
    mesh->translation(glm::vec3(0.0F, 7.0F, 0.0F));
    still->translation(glm::vec3(4.0F, 0.0F, 0.0F));
    scene.add(mesh);
    scene.add(still);
    scene.curve(mesh->id(), "translation.x", between(0.0F, 10.0F));

    // a pose sets what a curve drives, and leaves the rest of the item and every other item
    const std::uint64_t untouched = still->revision();
    scene.pose(11.0F);
    BOOST_CHECK(mesh->translation() == glm::vec3(10.0F, 7.0F, 0.0F));
    BOOST_CHECK_EQUAL(still->revision(), untouched);
    scene.pose(1.0F);
    BOOST_CHECK_EQUAL(mesh->translation().x, 0.0F);
}

BOOST_AUTO_TEST_CASE(scene_timeline_test) {
    v3d::editor::Scene scene;
    BOOST_CHECK(scene.timeline() == v3d::editor::Timeline());
    BOOST_CHECK(scene.timeline().valid());

    scene.timeline({ .rate = 30.0F, .start = 10, .end = 40, .frame = 12 });
    BOOST_CHECK_EQUAL(scene.timeline().frame, 12);

    // a closed document's timeline is the default one
    scene.clear();
    BOOST_CHECK(scene.timeline() == v3d::editor::Timeline());

    const v3d::editor::Timeline still{ .rate = 0.0F };
    const v3d::editor::Timeline backwards{ .start = 5, .end = 4 };
    const v3d::editor::Timeline outside{ .frame = 500 };
    BOOST_CHECK(!still.valid());
    BOOST_CHECK(!backwards.valid());
    BOOST_CHECK(!outside.valid());
}

BOOST_AUTO_TEST_CASE(scene_uppermost_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Group> inner = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> deep = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> loose = emptyMesh();
    scene.add(group);
    scene.add(inner);
    scene.add(deep);
    scene.add(loose);
    scene.parent(inner->id(), group->id());
    scene.parent(deep->id(), inner->id());

    // an item beneath another of the set, at any depth, is left out, and the order is kept
    const v3d::editor::Items top = scene.uppermost({ deep, loose, group, boost::shared_ptr<v3d::editor::Item>() });
    BOOST_REQUIRE_EQUAL(top.size(), 2U);
    BOOST_CHECK(top[0] == loose);
    BOOST_CHECK(top[1] == group);

    // and an item whose ancestors are not in the set is kept
    const v3d::editor::Items alone = scene.uppermost({ deep });
    BOOST_REQUIRE_EQUAL(alone.size(), 1U);
    BOOST_CHECK(alone[0] == deep);
}

};  // namespace
