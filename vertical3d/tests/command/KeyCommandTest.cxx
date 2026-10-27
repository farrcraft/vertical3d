/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/animation/Curve.h>
#include <vertical3d/src/command/KeyCommand.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

boost::shared_ptr<v3d::editor::Mesh> emptyMesh() {
    return boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
}

/**
 * Move the scene to a frame, as the timeline's transport does.
 **/
void goTo(v3d::editor::Scene* scene, int frame) {
    scene->frame(frame);
}

/**
 * Where an item is shown at the scene's current frame.
 **/
glm::vec3 shown(const v3d::editor::Scene& scene, const v3d::editor::Item& item) {
    return scene.evaluate().pose(item).placement.position;
}

/**
 * Key the items at the scene's frame, as Set Key does.
 **/
void key(const boost::shared_ptr<v3d::editor::Scene>& scene, const v3d::editor::Items& items) {
    v3d::editor::KeyCommand command(scene, items, static_cast<float>(scene->timeline().frame));
    command.redo();
}

BOOST_AUTO_TEST_CASE(key_nothing_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();

    // there is nothing to key in an empty set, or in items the scene does not hold
    const v3d::editor::KeyCommand none(scene, {}, 1.0F);
    BOOST_CHECK(none.empty());
    BOOST_CHECK(v3d::editor::KeyCommand(scene, { emptyMesh() }, 1.0F).empty());
    BOOST_CHECK_EQUAL(none.name(), "set key");
}

BOOST_AUTO_TEST_CASE(key_sets_and_undoes_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    mesh->translation(glm::vec3(1.0F, 2.0F, 3.0F));
    scene->add(mesh);

    // a key is set on every property the item declares, at the frame, with the item's value
    v3d::editor::KeyCommand command(scene, { mesh }, 10.0F);
    BOOST_CHECK(!command.empty());
    command.redo();
    const v3d::editor::Curves curves = scene->curves(mesh->id());
    BOOST_REQUIRE_EQUAL(curves.size(), v3d::editor::properties(v3d::editor::Item::Kind::Mesh).size());
    BOOST_REQUIRE_EQUAL(curves.at("translation.y").keys().size(), 1U);
    BOOST_CHECK_EQUAL(curves.at("translation.y").keys()[0].frame, 10.0F);
    BOOST_CHECK_EQUAL(curves.at("translation.y").keys()[0].value, 2.0F);
    BOOST_CHECK_EQUAL(curves.at("scale.x").keys()[0].value, 1.0F);

    // undo takes the curves away, and redo brings them back
    command.undo();
    BOOST_CHECK(scene->curves(mesh->id()).empty());
    command.redo();
    BOOST_CHECK(scene->curves(mesh->id()) == curves);
}

BOOST_AUTO_TEST_CASE(key_plays_back_between_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    const boost::shared_ptr<v3d::editor::Mesh> still = emptyMesh();
    scene->add(mesh);
    scene->add(still);
    still->translation(glm::vec3(5.0F, 0.0F, 0.0F));

    goTo(scene.get(), 1);
    key(scene, { mesh });
    goTo(scene.get(), 21);
    mesh->translation(glm::vec3(10.0F, 0.0F, 0.0F));
    key(scene, { mesh });

    // the frames between the keys ease from one to the other, and an item with no curves
    // keeps its place. The item keeps the value it was given
    goTo(scene.get(), 11);
    BOOST_CHECK_SMALL(shown(*scene, *mesh).x - 5.0F, 0.0001F);
    BOOST_CHECK(shown(*scene, *still) == glm::vec3(5.0F, 0.0F, 0.0F));
    goTo(scene.get(), 1);
    BOOST_CHECK_EQUAL(shown(*scene, *mesh).x, 0.0F);
    BOOST_CHECK_EQUAL(mesh->translation().x, 10.0F);

    // an edit shows until the frame is set again, and a key at the frame keeps it
    mesh->translation(glm::vec3(3.0F, 0.0F, 0.0F));
    BOOST_CHECK_EQUAL(shown(*scene, *mesh).x, 3.0F);
    goTo(scene.get(), 1);
    BOOST_CHECK_EQUAL(shown(*scene, *mesh).x, 0.0F);
    mesh->translation(glm::vec3(4.0F, 0.0F, 0.0F));
    v3d::editor::KeyCommand change(scene, { mesh }, 1.0F);
    change.redo();
    goTo(scene.get(), 21);
    goTo(scene.get(), 1);
    BOOST_CHECK_EQUAL(shown(*scene, *mesh).x, 4.0F);

    // undoing the key shows the curves as they were
    change.undo();
    BOOST_CHECK_EQUAL(shown(*scene, *mesh).x, 0.0F);
    BOOST_CHECK_EQUAL(scene->keyedFrames({ mesh }).size(), 2U);
}

BOOST_AUTO_TEST_CASE(key_rotation_keeps_turning_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    scene->add(mesh);
    mesh->rotation(v3d::editor::fromEuler(glm::vec3(0.0F, 0.0F, 170.0F)));
    goTo(scene.get(), 1);
    key(scene, { mesh });

    // twenty degrees further on is 190, not the -170 the same rotation is closest to zero
    goTo(scene.get(), 10);
    mesh->rotation(v3d::editor::fromEuler(glm::vec3(0.0F, 0.0F, 190.0F)));
    key(scene, { mesh });
    const v3d::editor::Curves curves = scene->curves(mesh->id());
    BOOST_REQUIRE_EQUAL(curves.at("rotation.z").keys().size(), 2U);
    BOOST_CHECK_SMALL(curves.at("rotation.z").keys()[1].value - 190.0F, 0.01F);
    BOOST_CHECK_SMALL(curves.at("rotation.x").keys()[1].value, 0.01F);
}

BOOST_AUTO_TEST_CASE(key_lens_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    scene->add(camera);
    camera->lens({ .fov = 30.0F });
    goTo(scene.get(), 1);
    key(scene, { camera });
    goTo(scene.get(), 5);
    camera->lens({ .fov = 50.0F });
    key(scene, { camera });

    // a lens is keyed as any other property, and zooms between its keys
    goTo(scene.get(), 3);
    BOOST_CHECK_SMALL(scene->evaluate().pose(*camera).lens.fov - 40.0F, 0.0001F);
    BOOST_CHECK_EQUAL(scene->curves(camera->id()).at("lens.fov").keys().size(), 2U);
}

};  // namespace
