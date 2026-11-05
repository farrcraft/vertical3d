/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/type/Transform.h>
#include <vertical3d/src/command/Parent.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

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

/**
 * A scene of a turned, scaled group and a mesh beside it, which is not beneath it.
 **/
struct Placed final {
    boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    boost::shared_ptr<v3d::editor::scene::item::Group> holder = boost::make_shared<v3d::editor::scene::item::Group>();
    boost::shared_ptr<v3d::editor::scene::item::Mesh> mesh =
        boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>());

    Placed() {
        holder->translation(glm::vec3(0.0F, 4.0F, 0.0F));
        holder->rotation(glm::angleAxis(glm::radians(90.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
        holder->scale(glm::vec3(2.0F));
        mesh->translation(glm::vec3(3.0F, 1.0F, 0.0F));
        mesh->rotation(glm::angleAxis(glm::radians(30.0F), glm::vec3(0.0F, 1.0F, 0.0F)));
        scene->add(holder);
        scene->add(mesh);
    }
};

boost::shared_ptr<v3d::editor::scene::item::Mesh> emptyMesh() {
    return boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>());
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

BOOST_AUTO_TEST_CASE(parent_keeps_the_world_place_test) {
    const Placed placed;
    const v3d::editor::scene::Evaluation before = placed.scene->evaluate();
    const v3d::type::Transform own = placed.mesh->placement();

    // the mesh goes beneath the group and stays where it was in the world
    v3d::editor::command::Parent command(placed.scene, { .item = placed.mesh, .parent = placed.holder });
    BOOST_CHECK(!command.empty());
    BOOST_CHECK_EQUAL(command.name(), "parent");
    command.redo();
    BOOST_CHECK(placed.scene->parent(placed.mesh->id()) == placed.holder);
    BOOST_CHECK(samePlace(placed.scene->evaluate().placed(placed.mesh->id()).world, before.placed(placed.mesh->id()).world));

    // and moving the group now moves it
    placed.holder->translate(glm::vec3(0.0F, 0.0F, 5.0F));
    const glm::vec3 moved(placed.scene->evaluate().placed(placed.mesh->id()).world[3]);
    BOOST_CHECK_SMALL(glm::distance(moved, glm::vec3(3.0F, 1.0F, 5.0F)), 0.0001F);
    placed.holder->translate(glm::vec3(0.0F, 0.0F, -5.0F));

    // undo leaves it with no parent and its own placement
    command.undo();
    BOOST_CHECK(!placed.scene->parent(placed.mesh->id()));
    BOOST_CHECK(v3d::editor::command::same(placed.mesh->placement(), own));
}

BOOST_AUTO_TEST_CASE(parent_to_none_test) {
    const Placed placed;
    placed.scene->parent(placed.mesh->id(), placed.holder->id());
    const v3d::editor::scene::Evaluation before = placed.scene->evaluate();

    // an item taken out from under its parent stays where it was in the world
    v3d::editor::command::Parent command(placed.scene, { .item = placed.mesh, .parent = nullptr });
    BOOST_CHECK(!command.empty());
    command.redo();
    BOOST_CHECK(!placed.scene->parent(placed.mesh->id()));
    BOOST_CHECK(samePlace(placed.scene->evaluate().placed(placed.mesh->id()).world, before.placed(placed.mesh->id()).world));

    command.undo();
    BOOST_CHECK(placed.scene->parent(placed.mesh->id()) == placed.holder);
    BOOST_CHECK(samePlace(placed.scene->evaluate().placed(placed.mesh->id()).world, before.placed(placed.mesh->id()).world));
}

BOOST_AUTO_TEST_CASE(parent_nothing_to_do_test) {
    const Placed placed;
    const boost::shared_ptr<v3d::editor::scene::item::Mesh> outside = emptyMesh();
    placed.scene->parent(placed.mesh->id(), placed.holder->id());

    // the parent it already has, itself, an item beneath it, and an item the scene does not
    // hold each change nothing
    BOOST_CHECK(v3d::editor::command::Parent(placed.scene, { .item = placed.mesh, .parent = placed.holder }).empty());
    BOOST_CHECK(v3d::editor::command::Parent(placed.scene, { .item = placed.holder, .parent = placed.holder }).empty());
    BOOST_CHECK(v3d::editor::command::Parent(placed.scene, { .item = placed.holder, .parent = placed.mesh }).empty());
    BOOST_CHECK(v3d::editor::command::Parent(placed.scene, { .item = placed.mesh, .parent = outside }).empty());
    BOOST_CHECK(v3d::editor::command::Parent(placed.scene, { .item = outside, .parent = placed.holder }).empty());

    v3d::editor::command::Parent loop(placed.scene, { .item = placed.holder, .parent = placed.mesh });
    loop.redo();
    BOOST_CHECK(!placed.scene->parent(placed.holder->id()));
}

};  // namespace
