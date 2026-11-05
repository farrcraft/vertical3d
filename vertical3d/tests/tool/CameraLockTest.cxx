/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/CameraLock.h>
#include <vertical3d/src/view/Through.h>
#include <vertical3d/src/view/ViewPort.h>

#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::tool::CameraControl;
using v3d::editor::view::ViewPort;

/**
 * A camera beneath a turned group, and a square view looking through it at a picture twice as
 * wide as it is tall, so the view shows more than the frame above and below.
 **/
struct Locked final {
    boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    boost::shared_ptr<v3d::editor::scene::item::Group> rig = boost::make_shared<v3d::editor::scene::item::Group>();
    boost::shared_ptr<v3d::editor::scene::item::Camera> camera = boost::make_shared<v3d::editor::scene::item::Camera>();
    boost::shared_ptr<ViewPort> view;

    Locked() {
        rig->translation(glm::vec3(1.0F, 0.0F, 0.0F));
        rig->rotation(glm::angleAxis(glm::radians(30.0F), glm::vec3(0.0F, 1.0F, 0.0F)));
        camera->translation(glm::vec3(0.0F, 1.0F, -6.0F));
        v3d::editor::scene::item::Camera::Lens lens = camera->lens();
        lens.fov = 35.0F;
        camera->lens(lens);
        scene->add(rig);
        scene->add(camera);
        scene->parent(camera->id(), rig->id());
        v3d::editor::scene::RenderSettings settings = scene->renderSettings();
        settings.size = glm::uvec2(400, 200);
        scene->renderSettings(settings);

        view = boost::make_shared<ViewPort>("view", v3d::type::camera::Profile("Perspective"));
        view->resize(glm::vec4(0.0F, 0.0F, 400.0F, 400.0F));
        view->through({ .camera = camera->documentId(), .scene = false, .locked = true });
        v3d::editor::view::follow(*scene, view.get());
    }
};

BOOST_AUTO_TEST_SUITE(camera_lock_test)

/**
 * Where a view through a camera puts the camera is where the camera is: the placement in its
 * parent's space, and the lens less the room around the frame.
 **/
BOOST_AUTO_TEST_CASE(a_view_puts_its_camera_where_it_is) {
    const Locked locked;
    const v3d::editor::command::Edit::Values placed = v3d::editor::tool::CameraLock::placed(*locked.scene, *locked.camera, *locked.view);
    BOOST_CHECK_SMALL(glm::distance(placed.pose.placement.position, locked.camera->translation()), 0.0001F);
    BOOST_CHECK_SMALL(glm::abs(glm::dot(placed.pose.placement.rotation, locked.camera->rotation())) - 1.0F, 0.0001F);
    BOOST_CHECK_CLOSE(placed.pose.lens.fov, 35.0F, 0.01F);
}

/**
 * A drag of a locked view moves its camera as it goes, and is one change when it ends, which
 * undo takes back.
 **/
BOOST_AUTO_TEST_CASE(a_locked_drag_moves_the_camera_as_one_change) {
    const Locked locked;
    const boost::shared_ptr<v3d::editor::command::Stack> history = boost::make_shared<v3d::editor::command::Stack>();
    v3d::editor::tool::CameraLock lock(history);
    CameraControl tool;
    std::vector<CameraControl::Navigated> told;
    tool.watch([&](ViewPort& view, CameraControl::Navigated what) {
        told.push_back(what);
        if (what == CameraControl::Navigated::Moved) {
            lock.moved(locked.scene, view);
        } else if (what == CameraControl::Navigated::Ended) {
            lock.ended();
        }
    });
    tool.view(locked.view);
    const glm::vec3 before = locked.camera->translation();

    tool.activate("truck");
    tool.button(1, true, glm::vec2(200.0F, 200.0F));
    tool.motion(glm::vec2(260.0F, 200.0F));
    tool.button(1, false, glm::vec2(260.0F, 200.0F));
    BOOST_CHECK(!locked.view->free());
    BOOST_CHECK(told == (std::vector<CameraControl::Navigated>{ CameraControl::Navigated::Moved, CameraControl::Navigated::Ended }));
    BOOST_CHECK_GT(glm::distance(locked.camera->translation(), before), 0.01F);
    BOOST_REQUIRE_EQUAL(history->undoDepth(), 1U);

    const boost::shared_ptr<v3d::editor::command::Command> undone = history->undo();
    BOOST_CHECK_EQUAL(undone->name(), "move camera");
    undone->undo();
    BOOST_CHECK_SMALL(glm::distance(locked.camera->translation(), before), 0.0001F);
}

/**
 * A drag of a view through a camera that is not locked frees the view before it moves it, and
 * leaves the camera where it was.
 **/
BOOST_AUTO_TEST_CASE(an_unlocked_drag_frees_the_view) {
    const Locked locked;
    locked.view->through({ .camera = locked.camera->documentId(), .scene = false, .locked = false });
    CameraControl tool;
    std::vector<CameraControl::Navigated> told;
    tool.watch([&told](ViewPort&, CameraControl::Navigated what) { told.push_back(what); });
    tool.view(locked.view);
    const glm::vec3 before = locked.camera->translation();

    tool.activate("truck");
    tool.button(1, true, glm::vec2(200.0F, 200.0F));
    tool.motion(glm::vec2(260.0F, 200.0F));
    tool.button(1, false, glm::vec2(260.0F, 200.0F));
    BOOST_CHECK(locked.view->free());
    BOOST_CHECK_EQUAL(locked.view->camera()->profile().name(), "Perspective");
    BOOST_CHECK(told == std::vector<CameraControl::Navigated>{ CameraControl::Navigated::Freed });
    BOOST_CHECK(locked.camera->translation() == before);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
