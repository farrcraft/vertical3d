/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <vertical3d/src/command/GroupCommand.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <cstdint>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

boost::shared_ptr<v3d::editor::Mesh> emptyMesh() {
    return boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
}

/**
 * Whether two matrices place everything in the same place, to within rounding.
 **/
bool samePlace(const glm::mat4& first, const glm::mat4& second) {
    for (int column = 0; column < 4; column++) {
        if (glm::distance(first[column], second[column]) > 0.0001F) {
            return false;
        }
    }
    return true;
}

/**
 * Where an item's origin is in the world.
 **/
glm::vec3 origin(const v3d::editor::Scene& scene, const v3d::editor::Item& item) {
    return glm::vec3(scene.evaluate().placed(item.id()).world[3]);
}

BOOST_AUTO_TEST_CASE(group_nothing_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> outside = emptyMesh();

    // there is nothing to group in an empty set, or in items the scene does not hold
    const v3d::editor::GroupCommand none(scene, {});
    BOOST_CHECK(none.empty());
    v3d::editor::GroupCommand stranger(scene, { outside });
    BOOST_CHECK(stranger.empty());
    stranger.redo();
    BOOST_CHECK_EQUAL(scene->count(), 0U);
    BOOST_CHECK_EQUAL(none.name(), "group");
}

BOOST_AUTO_TEST_CASE(group_stays_put_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Light> second = boost::make_shared<v3d::editor::Light>();
    const boost::shared_ptr<v3d::editor::Mesh> other = emptyMesh();
    first->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    first->rotation(glm::angleAxis(glm::radians(45.0F), glm::vec3(0.0F, 1.0F, 0.0F)));
    second->translation(glm::vec3(3.0F, 2.0F, 0.0F));
    scene->add(first);
    scene->add(second);
    scene->add(other);
    first->selected(true);
    second->selected(true);
    const v3d::editor::Evaluation before = scene->evaluate();

    v3d::editor::GroupCommand command(scene, scene->selection());
    command.redo();
    const boost::shared_ptr<v3d::editor::Group> group = command.group();

    // the group is placed at the mean of the origins, and holds the items, which have not
    // moved in the world
    BOOST_CHECK_EQUAL(scene->count(), 4U);
    BOOST_CHECK(scene->item(group->id()) == group);
    BOOST_CHECK_SMALL(glm::distance(group->translation(), glm::vec3(2.0F, 1.0F, 0.0F)), 0.0001F);
    BOOST_CHECK(scene->parent(first->id()) == group);
    BOOST_CHECK(scene->parent(second->id()) == group);
    BOOST_CHECK(!scene->parent(other->id()));
    const v3d::editor::Evaluation after = scene->evaluate();
    BOOST_CHECK(samePlace(after.placed(first->id()).world, before.placed(first->id()).world));
    BOOST_CHECK(samePlace(after.placed(second->id()).world, before.placed(second->id()).world));

    // the group becomes the selection
    const v3d::editor::Items selection = scene->selection();
    BOOST_REQUIRE_EQUAL(selection.size(), 1U);
    BOOST_CHECK(selection[0] == group);

    // and moving it moves what it holds
    group->translate(glm::vec3(0.0F, 0.0F, 5.0F));
    BOOST_CHECK_SMALL(glm::distance(origin(*scene, *first), glm::vec3(1.0F, 0.0F, 5.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(origin(*scene, *second), glm::vec3(3.0F, 2.0F, 5.0F)), 0.0001F);
}

BOOST_AUTO_TEST_CASE(group_undo_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> second = emptyMesh();
    first->translation(glm::vec3(-1.0F, 0.0F, 0.0F));
    second->translation(glm::vec3(4.0F, 0.0F, 0.0F));
    scene->add(first);
    scene->add(second);
    const v3d::editor::Placement firstBefore = v3d::editor::Placement::of(*first);
    const v3d::editor::Placement secondBefore = v3d::editor::Placement::of(*second);

    v3d::editor::GroupCommand command(scene, { first, second });
    command.redo();
    const unsigned int group = command.group()->id();
    const std::int64_t document = command.group()->documentId();

    // undo takes the group away, puts the items back where they were with no parent, and
    // leaves them selected, as they were when they were grouped
    command.undo();
    BOOST_CHECK_EQUAL(scene->count(), 2U);
    BOOST_CHECK(!scene->item(group));
    BOOST_CHECK(!scene->parent(first->id()));
    BOOST_CHECK(!scene->parent(second->id()));
    BOOST_CHECK(v3d::editor::Placement::of(*first).same(firstBefore));
    BOOST_CHECK(v3d::editor::Placement::of(*second).same(secondBefore));
    BOOST_CHECK(first->selected());
    BOOST_CHECK(second->selected());

    // redo brings back the same group, with the same ids
    command.redo();
    BOOST_CHECK(scene->item(group) == command.group());
    BOOST_CHECK_EQUAL(command.group()->documentId(), document);
    BOOST_CHECK(scene->parent(first->id()) == command.group());
    BOOST_CHECK(!first->selected());
}

BOOST_AUTO_TEST_CASE(group_shared_parent_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Group> holder = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> first = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> second = emptyMesh();
    holder->translation(glm::vec3(0.0F, 10.0F, 0.0F));
    holder->rotation(glm::angleAxis(glm::radians(90.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
    holder->scale(glm::vec3(2.0F, 1.0F, 1.0F));
    first->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    second->translation(glm::vec3(3.0F, 0.0F, 0.0F));
    scene->add(holder);
    scene->add(first);
    scene->add(second);
    scene->parent(first->id(), holder->id());
    scene->parent(second->id(), holder->id());
    const v3d::editor::Evaluation before = scene->evaluate();

    v3d::editor::GroupCommand command(scene, { first, second });
    command.redo();

    // items that share a parent are grouped beneath it, at the mean of their own
    // translations, and keep the rest of their placement exactly
    BOOST_CHECK(scene->parent(command.group()->id()) == holder);
    BOOST_CHECK(command.group()->translation() == glm::vec3(2.0F, 0.0F, 0.0F));
    BOOST_CHECK(first->translation() == glm::vec3(-1.0F, 0.0F, 0.0F));
    BOOST_CHECK(first->rotation() == glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK(first->scale() == glm::vec3(1.0F));
    const v3d::editor::Evaluation after = scene->evaluate();
    BOOST_CHECK(samePlace(after.placed(first->id()).world, before.placed(first->id()).world));
    BOOST_CHECK(samePlace(after.placed(second->id()).world, before.placed(second->id()).world));
}

BOOST_AUTO_TEST_CASE(group_mixed_parents_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Group> holder = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> held = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> loose = emptyMesh();
    holder->translation(glm::vec3(0.0F, 4.0F, 0.0F));
    holder->rotation(glm::angleAxis(glm::radians(90.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
    holder->scale(glm::vec3(2.0F));
    held->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    held->scale(glm::vec3(1.0F, 3.0F, 1.0F));
    loose->translation(glm::vec3(2.0F, 0.0F, 0.0F));
    scene->add(holder);
    scene->add(held);
    scene->add(loose);
    scene->parent(held->id(), holder->id());
    const v3d::editor::Evaluation before = scene->evaluate();

    v3d::editor::GroupCommand command(scene, { held, loose });
    command.redo();

    // items with different parents are grouped at the top of the scene, at the mean of
    // their world origins. The one taken from beneath a parent keeps its place in the world
    // through a placement of its own
    BOOST_CHECK(!scene->parent(command.group()->id()));
    BOOST_CHECK_SMALL(glm::distance(command.group()->translation(), glm::vec3(1.0F, 3.0F, 0.0F)), 0.0001F);
    const v3d::editor::Evaluation after = scene->evaluate();
    BOOST_CHECK(samePlace(after.placed(held->id()).world, before.placed(held->id()).world));
    BOOST_CHECK(samePlace(after.placed(loose->id()).world, before.placed(loose->id()).world));
    BOOST_CHECK_SMALL(glm::distance(held->scale(), glm::vec3(2.0F, 6.0F, 2.0F)), 0.0001F);

    // undo puts it back beneath its parent
    command.undo();
    BOOST_CHECK(scene->parent(held->id()) == holder);
    BOOST_CHECK(held->scale() == glm::vec3(1.0F, 3.0F, 1.0F));
}

BOOST_AUTO_TEST_CASE(group_nested_selection_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Group> holder = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> held = emptyMesh();
    scene->add(holder);
    scene->add(held);
    scene->parent(held->id(), holder->id());

    // an item beneath another of the set stays with that one
    v3d::editor::GroupCommand command(scene, { holder, held });
    command.redo();
    BOOST_CHECK(scene->parent(holder->id()) == command.group());
    BOOST_CHECK(scene->parent(held->id()) == holder);
    BOOST_CHECK_EQUAL(scene->children(command.group()->id()).size(), 1U);
}

};  // namespace
