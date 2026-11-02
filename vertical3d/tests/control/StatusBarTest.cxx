/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <api/log/Logger.h>
#include <api/ui/paint/Type.h>
#include <api/ui/tests/Typeset.h>
#include <vertical3d/src/control/StatusBar.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::control::Status;
using v3d::editor::control::StatusBar;

/**
 * A scene holding one cube, and the three tools over it.
 **/
struct Tools {
    Tools() :
        scene(boost::make_shared<v3d::editor::scene::Scene>()),
        cube(boost::make_shared<v3d::editor::scene::Mesh>(v3d::brep::cube())),
        select(scene, boost::make_shared<v3d::log::Logger>()),
        handles(scene, boost::make_shared<v3d::log::Logger>()) {
        scene->add(cube);
    }

    /**
     * @return what the left button does
     **/
    std::string left(bool perspective = true) const {
        const std::vector<Status::Button> buttons = StatusBar::buttons(camera, handles, select, perspective);
        BOOST_REQUIRE(!buttons.empty());
        BOOST_CHECK_EQUAL(buttons.front().name, "Left");
        return buttons.front().action;
    }

    boost::shared_ptr<v3d::editor::scene::Scene> scene;
    boost::shared_ptr<v3d::editor::scene::Mesh> cube;
    v3d::editor::tool::CameraControl camera;
    v3d::editor::tool::Select select;
    v3d::editor::tool::Transform handles;
};

BOOST_AUTO_TEST_SUITE(status_bar_test)

/**
 * The left button selects, adds to the selection and takes from it while that is held, and says
 * what the transform mode's handles do.
 **/
BOOST_FIXTURE_TEST_CASE(the_left_button_selects_or_moves, Tools) {
    BOOST_CHECK_EQUAL(left(), "Select");
    select.adding(true);
    BOOST_CHECK_EQUAL(left(), "Add or remove");
    select.adding(false);
    handles.activate("rotate");
    BOOST_CHECK_EQUAL(left(), "Select, or drag a handle to rotate");
    handles.activate("translate");
    BOOST_CHECK_EQUAL(left(), "Select, or drag a handle to move");
}

/**
 * A camera move held or latched takes the left button, and a held one wins over a latched one.
 **/
BOOST_FIXTURE_TEST_CASE(a_camera_move_takes_the_left_button, Tools) {
    camera.latch("dolly");
    BOOST_CHECK_EQUAL(left(), "Dolly");
    camera.activate("truck");
    BOOST_CHECK_EQUAL(left(), "Truck");
    camera.deactivate("truck");
    BOOST_CHECK_EQUAL(left(), "Dolly");
    camera.unlatch();
    camera.activate("pan");
    BOOST_CHECK_EQUAL(left(), "Orbit");
}

/**
 * The middle button turns a perspective camera, and is left out of an orthographic view.
 **/
BOOST_FIXTURE_TEST_CASE(the_middle_button_orbits_a_perspective_view, Tools) {
    const std::vector<Status::Button> perspective = StatusBar::buttons(camera, handles, select, true);
    BOOST_REQUIRE(!perspective.empty());
    BOOST_CHECK_EQUAL(perspective.back().name, "Middle");
    BOOST_CHECK_EQUAL(perspective.back().action, "Orbit");
    BOOST_CHECK_EQUAL(perspective.back().icon, "icon:mouse-middle");
    for (const Status::Button& button : StatusBar::buttons(camera, handles, select, false)) {
        BOOST_CHECK_NE(button.name, "Middle");
    }
}

/**
 * While the left button only selects, the keys held with it follow it, each with what it does.
 * While one of them is held, or a camera move takes the button, none follows.
 **/
BOOST_FIXTURE_TEST_CASE(the_keys_held_with_the_left_button_follow_it, Tools) {
    const std::vector<Status::Button> idle = StatusBar::buttons(camera, handles, select, false);
    BOOST_REQUIRE_EQUAL(idle.size(), 4U);
    BOOST_CHECK(idle[0].modifier.empty());
    BOOST_CHECK_EQUAL(idle[0].icon, "icon:mouse-left");
    BOOST_CHECK_EQUAL(idle[1].modifier, "Shift");
    BOOST_CHECK_EQUAL(idle[1].action, "Add or remove");
    BOOST_CHECK_EQUAL(idle[2].modifier, "Alt");
    BOOST_CHECK_EQUAL(idle[2].action, "Dolly");
    BOOST_CHECK_EQUAL(idle[3].modifier, "Ctrl");
    BOOST_CHECK_EQUAL(idle[3].action, "Truck");

    select.adding(true);
    BOOST_CHECK_EQUAL(StatusBar::buttons(camera, handles, select, false).size(), 1U);
    select.adding(false);
    camera.latch("dolly");
    BOOST_CHECK_EQUAL(StatusBar::buttons(camera, handles, select, false).size(), 1U);
}

/**
 * The frame is given against the timeline's last, and the frame rate's health against the
 * monitor's rate, or against 60 when that is not known.
 **/
BOOST_AUTO_TEST_CASE(the_frame_and_the_rates_health_read_plainly) {
    v3d::editor::scene::Timeline timeline;
    timeline.frame = 24;
    timeline.end = 120;
    BOOST_CHECK_EQUAL(StatusBar::frame(timeline), "Frame 24 of 120");

    BOOST_CHECK(StatusBar::health({ .mean = 16'666'667, .refresh = 60.0 }) == Status::Health::Good);
    BOOST_CHECK(StatusBar::health({ .mean = 25'000'000, .refresh = 60.0 }) == Status::Health::Low);
    BOOST_CHECK(StatusBar::health({ .mean = 50'000'000, .refresh = 60.0 }) == Status::Health::Poor);
    BOOST_CHECK(StatusBar::health({ .mean = 8'333'333, .refresh = 144.0 }) == Status::Health::Low);
    BOOST_CHECK(StatusBar::health({ .mean = 25'000'000, .refresh = 0.0 }) == Status::Health::Low);
    BOOST_CHECK(StatusBar::health({ .mean = 0, .refresh = 60.0 }) == Status::Health::Good);
}

/**
 * Under a mask that selects whole items, one item is named, with a mesh's vertices and faces, and
 * several are counted by kind. Under one that selects parts the count is of those parts. An edge
 * counts once whichever of its halves are selected.
 **/
BOOST_FIXTURE_TEST_CASE(the_selection_is_counted_under_the_mask, Tools) {
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Object), "Nothing selected");
    cube->selected(true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Object), "Mesh · Vertices 8 · Faces 6");
    cube->name("Box");
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Object), "Box · Vertices 8 · Faces 6");
    const boost::shared_ptr<v3d::editor::scene::Light> light = boost::make_shared<v3d::editor::scene::Light>();
    scene->add(light);
    light->selected(true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Object), "1 mesh · 1 light");
    const boost::shared_ptr<v3d::editor::scene::Mesh> second = boost::make_shared<v3d::editor::scene::Mesh>(v3d::brep::cube());
    scene->add(second);
    second->selected(true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Object), "2 meshes · 1 light");

    v3d::editor::scene::Components& parts = scene->components();
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Face), "Nothing selected");
    parts.select(cube->id(), v3d::editor::scene::Component::Face, 0, true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Face), "1 face");
    parts.select(cube->id(), v3d::editor::scene::Component::Vertex, 0, true);
    parts.select(cube->id(), v3d::editor::scene::Component::Vertex, 1, true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Vertex), "2 vertices");

    const v3d::brep::BRep& mesh = *cube->geometry();
    const v3d::brep::Index pair = mesh.edge(0)->pair();
    BOOST_REQUIRE(pair != v3d::brep::INVALID_ID);
    parts.select(cube->id(), v3d::editor::scene::Component::Edge, pair, true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Edge), "1 edge");
    parts.select(cube->id(), v3d::editor::scene::Component::Edge, 0, true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Edge), "1 edge");
    const v3d::brep::Index next = mesh.edge(0)->next();
    parts.select(cube->id(), v3d::editor::scene::Component::Edge, next, true);
    BOOST_CHECK_EQUAL(StatusBar::selection(*scene, v3d::editor::selection::SelectMask::Edge), "2 edges");
}

/**
 * A count names one thing in the singular, and the frame rate is rounded to a whole number.
 **/
BOOST_AUTO_TEST_CASE(counts_and_rates_read_plainly) {
    BOOST_CHECK_EQUAL(StatusBar::count(1, "vertex", "vertices"), "1 vertex");
    BOOST_CHECK_EQUAL(StatusBar::count(0, "vertex", "vertices"), "0 vertices");
    BOOST_CHECK_EQUAL(StatusBar::rate(16'666'667), "60 fps");
    BOOST_CHECK_EQUAL(StatusBar::rate(0), "");
}

/**
 * The frame rate's slot is as wide as three digits, whatever the rate is now, and grows only for
 * a rate wider than that.
 **/
BOOST_AUTO_TEST_CASE(the_rates_slot_does_not_follow_the_rate) {
    const v3d::ui::paint::Type figures = v3d::ui::typeset::body().tabular();
    const float slot = StatusBar::rateRoom(figures, "9 fps");
    BOOST_CHECK_EQUAL(slot, figures.width("000 fps"));
    BOOST_CHECK_EQUAL(StatusBar::rateRoom(figures, "60 fps"), slot);
    BOOST_CHECK_EQUAL(StatusBar::rateRoom(figures, "111 fps"), slot);
    BOOST_CHECK_EQUAL(StatusBar::rateRoom(figures, "888 fps"), slot);
    BOOST_CHECK_GT(StatusBar::rateRoom(figures, "8888 fps"), slot);
    BOOST_CHECK_EQUAL(StatusBar::rateRoom(v3d::ui::paint::Type(), "60 fps"), 0.0F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
