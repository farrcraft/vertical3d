/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/tool/CameraControlTool.h>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::editor::CameraControlTool CameraControlTool;

/**
 * Releasing a camera move ends that move only. With truck held after zoom, letting go of zoom
 * leaves the truck going, and letting go of truck stops it.
 **/
BOOST_AUTO_TEST_CASE(camera_control_tool_releases_only_its_own_move_test) {
    CameraControlTool tool;
    tool.activate("zoom");
    tool.activate("truck");

    tool.deactivate("zoom");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_TRUCK);

    tool.deactivate("truck");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_NONE);
}

};  // namespace
