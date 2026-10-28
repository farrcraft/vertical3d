/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/view/ViewPort.h>

#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

typedef v3d::editor::CameraControlTool CameraControlTool;

/**
 * A view four hundred pixels square, looking along +z from z of -10, perspective or not.
 **/
boost::shared_ptr<v3d::editor::ViewPort> view(bool orthographic) {
    v3d::type::camera::Profile profile("view");
    profile.orthographic(orthographic);
    profile.fov(60.0F);
    profile.orthoZoom(2.0F);
    profile.eye(glm::vec3(0.0F, 0.0F, -10.0F));
    const boost::shared_ptr<v3d::editor::ViewPort> result = boost::make_shared<v3d::editor::ViewPort>("view", profile);
    result->resize(glm::vec4(0.0F, 0.0F, 400.0F, 400.0F));
    return result;
}

/**
 * Drag the primary button from the middle of the view by an offset, under a move.
 **/
void drag(CameraControlTool* tool, const glm::vec2& offset) {
    const glm::vec2 middle(200.0F, 200.0F);
    tool->button(1, true, middle);
    tool->motion(middle + offset);
    tool->button(1, false, middle + offset);
}

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

BOOST_AUTO_TEST_CASE(camera_control_tool_latch_test) {
    CameraControlTool tool;

    // a latched move is in force with nothing held
    tool.latch("pitch");
    BOOST_CHECK(tool.latched() == CameraControlTool::CAMERA_MODE_PITCH);
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_PITCH);

    // a held move wins while it is held, and letting it go leaves the latch
    tool.activate("truck");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_TRUCK);
    tool.deactivate("truck");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_PITCH);

    // and a held release of the latched move's own name does not undo the latch
    tool.deactivate("pitch");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_PITCH);

    // latching another move moves the latch, and latching it again releases it
    tool.latch("dolly");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_DOLLY);
    tool.latch("dolly");
    BOOST_CHECK(tool.mode() == CameraControlTool::CAMERA_MODE_NONE);

    tool.latch("zoom");
    tool.unlatch();
    BOOST_CHECK(tool.latched() == CameraControlTool::CAMERA_MODE_NONE);
}

BOOST_AUTO_TEST_CASE(camera_control_tool_perspective_zoom_and_dolly_test) {
    CameraControlTool tool;
    const boost::shared_ptr<v3d::editor::ViewPort> perspective = view(false);
    const boost::shared_ptr<v3d::type::camera::Camera> camera = perspective->camera();
    tool.view(perspective);

    // a zoom to the right narrows the angle of view and leaves the eye where it was
    tool.latch("zoom");
    drag(&tool, glm::vec2(40.0F, 0.0F));
    BOOST_CHECK_CLOSE(camera->profile().fov(), 50.0F, 0.01F);
    BOOST_CHECK_SMALL(glm::distance(camera->profile().eye(), glm::vec3(0.0F, 0.0F, -10.0F)), 0.0001F);

    // and stops at the narrowest
    drag(&tool, glm::vec2(400.0F, 0.0F));
    BOOST_CHECK_CLOSE(camera->profile().fov(), CameraControlTool::narrowest, 0.01F);

    // a dolly moves the eye along the view and leaves the angle alone
    tool.latch("dolly");
    drag(&tool, glm::vec2(40.0F, 0.0F));
    BOOST_CHECK_CLOSE(camera->profile().fov(), CameraControlTool::narrowest, 0.01F);
    BOOST_CHECK_GT(camera->profile().eye().z, -10.0F);
    BOOST_CHECK_SMALL(camera->profile().eye().x, 0.0001F);
}

BOOST_AUTO_TEST_CASE(camera_control_tool_orthographic_zoom_and_dolly_test) {
    CameraControlTool tool;
    const boost::shared_ptr<v3d::editor::ViewPort> orthographic = view(true);
    const boost::shared_ptr<v3d::type::camera::Camera> camera = orthographic->camera();
    tool.view(orthographic);

    // both zoom and dolly change what an orthographic view shows, and neither moves its eye
    for (const std::string move : { "zoom", "dolly" }) {
        BOOST_TEST_CONTEXT(move) {
            const float before = camera->profile().orthoZoom();
            tool.latch(move);
            drag(&tool, glm::vec2(40.0F, 0.0F));
            BOOST_CHECK_LT(camera->profile().orthoZoom(), before);
            BOOST_CHECK_SMALL(glm::distance(camera->profile().eye(), glm::vec3(0.0F, 0.0F, -10.0F)), 0.0001F);
            BOOST_CHECK_CLOSE(camera->profile().fov(), 60.0F, 0.01F);
            tool.unlatch();
        }
    }
}

BOOST_AUTO_TEST_CASE(camera_control_tool_pitch_test) {
    CameraControlTool tool;
    const boost::shared_ptr<v3d::editor::ViewPort> perspective = view(false);
    tool.view(perspective);

    // a drag up turns a perspective camera to look up, and leaves its eye
    tool.latch("pitch");
    drag(&tool, glm::vec2(0.0F, -60.0F));
    const glm::vec3 direction = glm::normalize(perspective->camera()->profile().direction());
    BOOST_CHECK_GT(direction.y, 0.1F);
    BOOST_CHECK_SMALL(direction.x, 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(perspective->camera()->profile().eye(), glm::vec3(0.0F, 0.0F, -10.0F)), 0.0001F);

    // an orthographic view is not turned
    const boost::shared_ptr<v3d::editor::ViewPort> orthographic = view(true);
    tool.view(orthographic);
    drag(&tool, glm::vec2(0.0F, -60.0F));
    BOOST_CHECK_SMALL(glm::distance(orthographic->camera()->profile().direction(), glm::vec3(0.0F, 0.0F, 1.0F)), 0.0001F);
}

};  // namespace
