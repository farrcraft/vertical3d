/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/command/Units.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Units.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(units_command_test) {
    const boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    BOOST_CHECK(scene->length() == v3d::editor::scene::Length::Metre);

    // the unit it is made from is the one it puts back
    v3d::editor::command::Units command(scene, v3d::editor::scene::Length::Inch);
    command.redo();
    BOOST_CHECK(scene->length() == v3d::editor::scene::Length::Inch);
    command.undo();
    BOOST_CHECK(scene->length() == v3d::editor::scene::Length::Metre);
    BOOST_CHECK_EQUAL(command.name(), "unit of length");
}

};  // namespace
