/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/type/Transform.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Keyed.h>
#include <vertical3d/src/command/Transform.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

/**
 * A scene holding one empty mesh, at frame 5, with auto key on or off.
 **/
struct Keying final {
    explicit Keying(bool on) :
        scene(boost::make_shared<v3d::editor::scene::Scene>()),
        mesh(boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>())) {
        scene->add(mesh);
        scene->timeline({ .frame = 5, .autoKey = on });
    }

    /**
     * Move the mesh as a manipulator does, and record the gesture through auto key.
     **/
    boost::shared_ptr<v3d::editor::command::Command> place(const v3d::type::Transform& to) {
        const v3d::type::Transform from = mesh->placement();
        const v3d::editor::scene::Pose before = v3d::editor::scene::pose(*mesh);
        mesh->placement(to);
        edit = boost::make_shared<v3d::editor::command::Transform>(
            std::vector<v3d::editor::command::Transform::Change>{ { .item = mesh, .before = from, .after = to } }, "translate");
        return v3d::editor::command::Keyed::record(scene, edit, { { .item = mesh, .before = before, .after = v3d::editor::scene::pose(*mesh) } });
    }

    boost::shared_ptr<v3d::editor::scene::Scene> scene;
    boost::shared_ptr<v3d::editor::scene::item::Mesh> mesh;
    // the edit place() last made, before auto key saw it
    boost::shared_ptr<v3d::editor::command::Command> edit;
};

BOOST_AUTO_TEST_CASE(keyed_off_test) {
    Keying keying(false);
    v3d::type::Transform moved;
    moved.position = glm::vec3(2.0F, 0.0F, 0.0F);

    // with auto key off the edit is recorded alone, and nothing is keyed
    BOOST_CHECK(keying.place(moved) == keying.edit);
    BOOST_CHECK(keying.scene->curves(keying.mesh->id()).empty());
}

BOOST_AUTO_TEST_CASE(keyed_on_test) {
    Keying keying(true);
    v3d::type::Transform moved;
    moved.position = glm::vec3(2.0F, 0.0F, 0.0F);
    const boost::shared_ptr<v3d::editor::command::Command> command = keying.place(moved);

    // the edit and its key are one command, named as the edit is, which keys only what moved
    BOOST_REQUIRE(command);
    BOOST_CHECK(command != keying.edit);
    BOOST_CHECK_EQUAL(command->name(), "translate");
    const v3d::editor::scene::Curves curves = keying.scene->curves(keying.mesh->id());
    BOOST_REQUIRE_EQUAL(curves.size(), 1U);
    BOOST_REQUIRE_EQUAL(curves.at("translation.x").keys().size(), 1U);
    BOOST_CHECK_EQUAL(curves.at("translation.x").keys()[0].frame, 5.0F);
    BOOST_CHECK_EQUAL(curves.at("translation.x").keys()[0].value, 2.0F);

    // undo takes back the key and the edit together, and redo makes both again
    command->undo();
    BOOST_CHECK(keying.scene->curves(keying.mesh->id()).empty());
    BOOST_CHECK_EQUAL(keying.mesh->translation().x, 0.0F);
    command->redo();
    BOOST_CHECK(keying.scene->curves(keying.mesh->id()) == curves);
    BOOST_CHECK_EQUAL(keying.mesh->translation().x, 2.0F);
}

BOOST_AUTO_TEST_CASE(keyed_rotation_test) {
    Keying keying(true);
    v3d::type::Transform turned;
    turned.rotation = v3d::editor::scene::fromEuler(glm::vec3(0.0F, 0.0F, 30.0F));
    keying.place(turned);

    // a turn about one axis keys the whole rotation, and nothing else
    const v3d::editor::scene::Curves curves = keying.scene->curves(keying.mesh->id());
    BOOST_CHECK_EQUAL(curves.size(), 3U);
    BOOST_CHECK(curves.contains("rotation.x") && curves.contains("rotation.y") && curves.contains("rotation.z"));
    BOOST_CHECK_SMALL(curves.at("rotation.z").keys()[0].value - 30.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(keyed_nothing_driven_test) {
    const Keying keying(true);
    const v3d::editor::command::Edit::Values before = v3d::editor::command::Edit::of(*keying.mesh);
    keying.mesh->name("Renamed");
    const boost::shared_ptr<v3d::editor::command::Command> edit = boost::make_shared<v3d::editor::command::Edit>(keying.mesh, before, "rename");

    // a name is not a number a curve drives, so a rename is recorded alone
    const boost::shared_ptr<v3d::editor::command::Command> command = v3d::editor::command::Keyed::record(keying.scene, edit,
        { { .item = keying.mesh, .before = before.pose, .after = v3d::editor::scene::pose(*keying.mesh) } });
    BOOST_CHECK(command == edit);
    BOOST_CHECK(keying.scene->curves(keying.mesh->id()).empty());
}

};  // namespace
