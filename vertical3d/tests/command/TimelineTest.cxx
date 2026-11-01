/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/command/Timeline.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(timeline_command_test) {
    const boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    scene->timeline({ .frame = 7 });

    // it sets the rate, the range and auto key, names itself as it is told, and puts them back
    v3d::editor::command::Timeline command(scene, { .rate = 30.0F, .start = 1, .end = 48, .frame = 40, .autoKey = true }, "frame range");
    command.redo();
    BOOST_CHECK_EQUAL(scene->timeline().rate, 30.0F);
    BOOST_CHECK_EQUAL(scene->timeline().end, 48);
    BOOST_CHECK(scene->timeline().autoKey);
    BOOST_CHECK_EQUAL(command.name(), "frame range");

    // the frame is not part of the change
    BOOST_CHECK_EQUAL(scene->timeline().frame, 7);
    command.undo();
    BOOST_CHECK(scene->timeline() == v3d::editor::scene::Timeline({ .frame = 7 }));
}

BOOST_AUTO_TEST_CASE(timeline_command_holds_the_frame_test) {
    const boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    scene->timeline({ .frame = 100 });

    // a range that no longer holds the frame moves it to the nearest end, and undo leaves it there
    v3d::editor::command::Timeline command(scene, { .start = 10, .end = 50 }, "frame range");
    command.redo();
    BOOST_CHECK_EQUAL(scene->timeline().frame, 50);
    BOOST_CHECK(scene->timeline().valid());
    command.undo();
    BOOST_CHECK_EQUAL(scene->timeline().end, v3d::editor::scene::Timeline::DEFAULT_END);
    BOOST_CHECK_EQUAL(scene->timeline().frame, 50);
}

};  // namespace
