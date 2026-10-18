/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ViewPort.h>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * The same square front view the picker's own tests use: a world unit is two hundred
 * pixels and the middle of the view is the middle of a unit primitive.
 **/
boost::shared_ptr<v3d::editor::ViewPort> frontView() {
    const v3d::type::camera::Profile profile("front");
    boost::shared_ptr<v3d::editor::ViewPort> view = boost::make_shared<v3d::editor::ViewPort>("front", profile);
    view->resize(glm::vec4(0.0F, 0.0F, 400.0F, 400.0F));
    return view;
}

/**
 * A tool over a scene, in a front view, with no logger.
 **/
boost::shared_ptr<v3d::editor::TransformTool> tool(const boost::shared_ptr<v3d::editor::Scene>& scene) {
    boost::shared_ptr<v3d::editor::TransformTool> transform =
        boost::make_shared<v3d::editor::TransformTool>(scene, boost::shared_ptr<v3d::log::Logger>());
    transform->view(frontView());
    return transform;
}

constexpr glm::vec2 centre(200.0F, 200.0F);

BOOST_AUTO_TEST_CASE(transform_tool_mode_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);

    // nothing is drawn until a mode asks for it, and the tool starts in the one that draws
    // nothing
    BOOST_CHECK(transform->mode() == v3d::editor::TransformTool::Mode::None);
    BOOST_CHECK(!transform->manipulator());

    transform->activate("translate");
    BOOST_CHECK(transform->mode() == v3d::editor::TransformTool::Mode::Translate);
    BOOST_CHECK(transform->manipulator());

    transform->activate("rotate");
    BOOST_CHECK(transform->mode() == v3d::editor::TransformTool::Mode::Rotate);

    transform->activate("scale");
    BOOST_CHECK(transform->mode() == v3d::editor::TransformTool::Mode::Scale);

    transform->activate("select");
    BOOST_CHECK(transform->mode() == v3d::editor::TransformTool::Mode::None);

    // a name that is not a mode leaves the tool where it was
    transform->activate("translate");
    transform->activate("extrude");
    BOOST_CHECK(transform->mode() == v3d::editor::TransformTool::Mode::Translate);
}

BOOST_AUTO_TEST_CASE(transform_tool_drag_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->activate("translate");

    // a press on the x handle grabs it, and the press goes to this tool rather than to the
    // one that selects
    transform->button(1, true, glm::vec2(centre.x + 45.0F, centre.y));
    BOOST_CHECK_EQUAL(transform->dragging(), true);
    BOOST_CHECK(transform->manipulator()->axis() == v3d::editor::Manipulator::Axis::X);

    transform->motion(glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.5F);

    // and the release ends it
    transform->button(1, false, glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_EQUAL(transform->dragging(), false);

    // a motion after the release moves nothing
    transform->motion(glm::vec2(centre.x, centre.y));
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.5F);
}

BOOST_AUTO_TEST_CASE(transform_tool_miss_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->activate("translate");

    // a press away from every handle is not a drag, so the same press can go on to select
    transform->button(1, true, glm::vec2(30.0F, 370.0F));
    BOOST_CHECK_EQUAL(transform->dragging(), false);
    transform->motion(glm::vec2(60.0F, 340.0F));
    BOOST_CHECK_SMALL(cube->translation().x, 0.001F);

    // neither is a press with nothing selected, however good the aim
    scene->deselect();
    transform->button(1, true, centre);
    BOOST_CHECK_EQUAL(transform->dragging(), false);
}

BOOST_AUTO_TEST_CASE(transform_tool_mode_change_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->activate("translate");
    transform->button(1, true, glm::vec2(centre.x + 45.0F, centre.y));
    BOOST_CHECK_EQUAL(transform->dragging(), true);

    // the handle being dragged is about to stop existing, so the drag goes with it
    transform->activate("rotate");
    BOOST_CHECK_EQUAL(transform->dragging(), false);
    transform->motion(glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_SMALL(cube->translation().x, 0.001F);
}

BOOST_AUTO_TEST_CASE(transform_tool_history_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::CommandStack> history = boost::make_shared<v3d::editor::CommandStack>();
    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->commands(history);
    transform->activate("translate");

    // one gesture is one undoable unit, however many motion events it took
    transform->button(1, true, glm::vec2(centre.x + 45.0F, centre.y));
    transform->motion(glm::vec2(centre.x + 95.0F, centre.y));
    transform->motion(glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_EQUAL(history->undoDepth(), 0U);

    transform->button(1, false, glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_EQUAL(history->undoDepth(), 1U);
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.5F);

    const boost::shared_ptr<v3d::editor::Command> undone = history->undo();
    BOOST_CHECK(undone);
    BOOST_CHECK_EQUAL(undone->name(), "translate");
    BOOST_CHECK_SMALL(cube->translation().x, 0.001F);

    history->redo();
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.5F);
}

BOOST_AUTO_TEST_CASE(transform_tool_no_gesture_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::CommandStack> history = boost::make_shared<v3d::editor::CommandStack>();
    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->commands(history);
    transform->activate("translate");

    // a handle grabbed and let go without moving changed nothing, so there is nothing to undo
    transform->button(1, true, glm::vec2(centre.x + 45.0F, centre.y));
    transform->button(1, false, glm::vec2(centre.x + 45.0F, centre.y));
    BOOST_CHECK_EQUAL(history->undoDepth(), 0U);

    // neither does a press that took no handle
    transform->button(1, true, glm::vec2(30.0F, 370.0F));
    transform->button(1, false, glm::vec2(30.0F, 370.0F));
    BOOST_CHECK_EQUAL(history->undoDepth(), 0U);
}

BOOST_AUTO_TEST_CASE(transform_tool_abandoned_gesture_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::CommandStack> history = boost::make_shared<v3d::editor::CommandStack>();
    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->commands(history);
    transform->activate("translate");

    transform->button(1, true, glm::vec2(centre.x + 45.0F, centre.y));
    transform->motion(glm::vec2(centre.x + 145.0F, centre.y));

    // the mode change drops the drag, but what it already wrote to the mesh stays, so it
    // has to be recorded in the history
    transform->activate("rotate");
    BOOST_CHECK_EQUAL(history->undoDepth(), 1U);
    BOOST_CHECK_EQUAL(history->undo()->name(), "translate");
    BOOST_CHECK_SMALL(cube->translation().x, 0.001F);
}

};  // namespace
