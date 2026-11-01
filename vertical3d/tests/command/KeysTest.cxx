/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <vertical3d/src/command/Keys.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A scene holding one mesh with two curves: translation.x keyed on frames 1 and 10, and
 * scale.x keyed on frame 10 alone.
 **/
struct Keyed final {
    Keyed() :
        scene(boost::make_shared<v3d::editor::scene::Scene>()),
        mesh(boost::make_shared<v3d::editor::scene::Mesh>(boost::make_shared<v3d::brep::BRep>())) {
        scene->add(mesh);
        v3d::animation::Curve moving;
        moving.set(1.0F, 0.0F);
        moving.set(10.0F, 5.0F);
        v3d::animation::Curve growing;
        growing.set(10.0F, 2.0F);
        scene->curve(mesh->id(), "translation.x", moving);
        scene->curve(mesh->id(), "scale.x", growing);
    }

    std::vector<float> frames(const char* path) const {
        std::vector<float> found;
        for (const v3d::animation::Curve::Key& key : scene->curves(mesh->id()).at(path).keys()) {
            found.push_back(key.frame);
        }
        return found;
    }

    boost::shared_ptr<v3d::editor::scene::Scene> scene;
    boost::shared_ptr<v3d::editor::scene::Mesh> mesh;
};

BOOST_AUTO_TEST_CASE(keys_move_test) {
    const Keyed keyed;
    const v3d::editor::scene::Curves before = keyed.scene->curves(keyed.mesh->id());

    // a key on frame 10 moves on every curve of its item, keeping its values
    const boost::shared_ptr<v3d::editor::command::Keys> command =
        v3d::editor::command::Keys::shift(keyed.scene, { { .item = keyed.mesh->id(), .frame = 10.0F } }, 3.0F);
    BOOST_CHECK(!command->empty());
    BOOST_CHECK_EQUAL(command->count(), 1U);
    BOOST_CHECK_EQUAL(command->name(), "move keys");
    command->redo();
    BOOST_CHECK(keyed.frames("translation.x") == std::vector<float>({ 1.0F, 13.0F }));
    BOOST_CHECK(keyed.frames("scale.x") == std::vector<float>({ 13.0F }));
    BOOST_CHECK_EQUAL(keyed.scene->curves(keyed.mesh->id()).at("scale.x").keys()[0].value, 2.0F);

    command->undo();
    BOOST_CHECK(keyed.scene->curves(keyed.mesh->id()) == before);
}

BOOST_AUTO_TEST_CASE(keys_remove_test) {
    const Keyed keyed;

    // a key on frame 10 goes from every curve, and a curve left with none goes too
    const boost::shared_ptr<v3d::editor::command::Keys> command =
        v3d::editor::command::Keys::remove(keyed.scene, { { .item = keyed.mesh->id(), .frame = 10.0F } });
    BOOST_CHECK_EQUAL(command->name(), "delete keys");
    command->redo();
    BOOST_CHECK(keyed.frames("translation.x") == std::vector<float>({ 1.0F }));
    BOOST_CHECK(!keyed.scene->curves(keyed.mesh->id()).contains("scale.x"));

    command->undo();
    BOOST_CHECK(keyed.frames("scale.x") == std::vector<float>({ 10.0F }));
    BOOST_CHECK(keyed.frames("translation.x") == std::vector<float>({ 1.0F, 10.0F }));
}

BOOST_AUTO_TEST_CASE(keys_nothing_test) {
    const Keyed keyed;

    // a frame with no keys, or an item the scene does not hold, gives nothing to change
    BOOST_CHECK(v3d::editor::command::Keys::shift(keyed.scene, { { .item = keyed.mesh->id(), .frame = 4.0F } }, 1.0F)->empty());
    BOOST_CHECK(v3d::editor::command::Keys::remove(keyed.scene, { { .item = 99, .frame = 1.0F } })->empty());
    BOOST_CHECK_EQUAL(v3d::editor::command::Keys::remove(keyed.scene, {})->count(), 0U);
}

};  // namespace
