/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/log/Logger.h>
#include <api/type/Transform.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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

/**
 * The tool takes a press on a handle of the selection, and no other, so a router offers the
 * rest to the select tool. Asking changes nothing: no drag starts.
 **/
BOOST_AUTO_TEST_CASE(transform_tool_takes_a_press_on_a_handle_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::TransformTool> transform = tool(scene);
    transform->activate("translate");

    BOOST_CHECK(transform->takes(glm::vec2(centre.x + 45.0F, centre.y)));
    BOOST_CHECK(!transform->takes(glm::vec2(30.0F, 370.0F)));
    BOOST_CHECK(!transform->dragging());

    // with no mode there are no handles, and with nothing selected there is nothing to move
    transform->activate("select");
    BOOST_CHECK(!transform->takes(glm::vec2(centre.x + 45.0F, centre.y)));
    transform->activate("translate");
    scene->deselect();
    BOOST_CHECK(!transform->takes(glm::vec2(centre.x + 45.0F, centre.y)));
}

BOOST_AUTO_TEST_CASE(transform_tool_mode_change_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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

/**
 * Two selected cubes either side of the middle of the front view, so the handles sit at
 * the middle, and a stack recording what the tool does to them.
 **/
struct TwoCubes {
    TwoCubes() :
        scene(boost::make_shared<v3d::editor::Scene>()),
        left(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        right(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        history(boost::make_shared<v3d::editor::CommandStack>()),
        transformTool(tool(scene)) {
        left->translation(glm::vec3(-0.5F, 0.0F, 0.0F));
        right->translation(glm::vec3(0.5F, 0.0F, 0.0F));
        left->selected(true);
        right->selected(true);
        scene->add(left);
        scene->add(right);
        transformTool->commands(history);
    }

    /**
     * Grab at one point, drag to another and let go, then check that the gesture is one
     * command that undoes and redoes both cubes exactly.
     **/
    void gesture(const std::string& mode, const glm::vec2& from, const glm::vec2& to) const {
        transformTool->activate(mode);
        const v3d::type::Transform leftBefore = left->placement();
        const v3d::type::Transform rightBefore = right->placement();
        const std::size_t depth = history->undoDepth();

        transformTool->motion(from);
        transformTool->button(1, true, from);
        transformTool->motion(to);
        transformTool->button(1, false, to);
        BOOST_REQUIRE_EQUAL(history->undoDepth(), depth + 1);

        const v3d::type::Transform leftAfter = left->placement();
        const v3d::type::Transform rightAfter = right->placement();
        BOOST_CHECK(!v3d::editor::same(leftAfter, leftBefore));
        BOOST_CHECK(!v3d::editor::same(rightAfter, rightBefore));

        BOOST_CHECK_EQUAL(history->undo()->name(), mode);
        BOOST_CHECK(v3d::editor::same(left->placement(), leftBefore));
        BOOST_CHECK(v3d::editor::same(right->placement(), rightBefore));
        history->redo();
        BOOST_CHECK(v3d::editor::same(left->placement(), leftAfter));
        BOOST_CHECK(v3d::editor::same(right->placement(), rightAfter));
    }

    boost::shared_ptr<v3d::editor::Scene> scene;
    boost::shared_ptr<v3d::editor::Mesh> left;
    boost::shared_ptr<v3d::editor::Mesh> right;
    boost::shared_ptr<v3d::editor::CommandStack> history;
    boost::shared_ptr<v3d::editor::TransformTool> transformTool;
};

BOOST_AUTO_TEST_CASE(transform_tool_set_translate_test) {
    const TwoCubes cubes;
    cubes.gesture("translate", glm::vec2(centre.x + 45.0F, centre.y), glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_SMALL(cubes.left->translation().x, 0.01F);
    BOOST_CHECK_CLOSE(cubes.right->translation().x, 1.0F, 1.0F);
}

BOOST_AUTO_TEST_CASE(transform_tool_set_rotate_test) {
    const TwoCubes cubes;
    // the z ring, which faces this view, is ninety pixels round the pivot
    cubes.gesture("rotate", glm::vec2(centre.x + 90.0F, centre.y), glm::vec2(centre.x, centre.y + 90.0F));
    BOOST_CHECK_CLOSE(cubes.left->translation().y, 0.5F, 0.5F);
    BOOST_CHECK_CLOSE(cubes.right->translation().y, -0.5F, 0.5F);
}

BOOST_AUTO_TEST_CASE(transform_tool_set_scale_test) {
    const TwoCubes cubes;
    cubes.gesture("scale", glm::vec2(centre.x + 45.0F, centre.y), glm::vec2(centre.x + 135.0F, centre.y));
    BOOST_CHECK_CLOSE(cubes.left->translation().x, -1.0F, 1.0F);
    BOOST_CHECK_CLOSE(cubes.right->translation().x, 1.0F, 1.0F);
}

BOOST_AUTO_TEST_CASE(transform_tool_set_kept_test) {
    const TwoCubes cubes;
    cubes.transformTool->activate("translate");

    // a drag moves the set it grabbed, whatever happens to the selection mid gesture
    cubes.transformTool->button(1, true, glm::vec2(centre.x + 45.0F, centre.y));
    cubes.right->selected(false);
    cubes.transformTool->motion(glm::vec2(centre.x + 145.0F, centre.y));
    cubes.transformTool->button(1, false, glm::vec2(centre.x + 145.0F, centre.y));
    BOOST_CHECK_CLOSE(cubes.right->translation().x, 1.0F, 1.0F);
    cubes.history->undo();
    BOOST_CHECK_CLOSE(cubes.right->translation().x, 0.5F, 0.5F);
}

/**
 * Where a world point lands in the front view.
 **/
glm::vec2 screen(const v3d::editor::ViewPort& view, const glm::vec3& point) {
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();
    camera->createProjection();
    camera->createView();
    int viewport[4] = { 0, 0, 400, 400 };
    const glm::vec3 projected = camera->project(point, viewport);
    return glm::vec2(projected[0], projected[1]);
}

/**
 * Every point and face normal of a mesh, to compare a mesh with itself exactly.
 **/
struct Snapshot {
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;

    explicit Snapshot(const v3d::brep::BRep& mesh) {
        for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
            points.push_back(mesh.vertex(id)->point());
        }
        for (v3d::brep::Index id = 0; id < mesh.faceCount(); id++) {
            normals.push_back(mesh.face(id)->normal());
        }
    }

    bool operator==(const Snapshot& other) const {
        return points == other.points && normals == other.normals;
    }
};

/**
 * A selected cube with some of its front components selected, a tool following the mask,
 * and a stack recording what the tool does.
 **/
struct Components {
    explicit Components(v3d::editor::SelectMask mask) :
        scene(boost::make_shared<v3d::editor::Scene>()),
        cube(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        history(boost::make_shared<v3d::editor::CommandStack>()),
        transformTool(tool(scene)) {
        cube->selected(true);
        scene->add(cube);
        transformTool->commands(history);
        transformTool->mask(mask);
        transformTool->activate("translate");
    }

    /**
     * Grab the x handle beside the pivot and drag it a hundred pixels right, which is half a
     * world unit.
     **/
    void drag() const {
        const glm::vec2 handle = screen(*transformTool->view(), transformTool->target().pivot()) + glm::vec2(45.0F, 0.0F);
        transformTool->motion(handle);
        transformTool->button(1, true, handle);
        transformTool->motion(handle + glm::vec2(100.0F, 0.0F));
        transformTool->button(1, false, handle + glm::vec2(100.0F, 0.0F));
    }

    /**
     * Drag, then check that only the given vertices moved, each by half a unit along x,
     * that the transform did not move, and that undo and redo restore the mesh exactly.
     **/
    void check(const std::vector<v3d::brep::Index>& moved) const {
        const Snapshot before(*cube->geometry());
        const v3d::type::Transform placed = cube->placement();
        drag();
        BOOST_REQUIRE_EQUAL(history->undoDepth(), 1U);
        const Snapshot after(*cube->geometry());

        for (v3d::brep::Index id = 0; id < cube->geometry()->vertexCount(); id++) {
            const bool chosen = std::find(moved.begin(), moved.end(), id) != moved.end();
            const glm::vec3 shift = after.points[id] - before.points[id];
            BOOST_CHECK_CLOSE(shift.x + 1.0F, chosen ? 1.5F : 1.0F, 1.0F);
            BOOST_CHECK_SMALL(shift.y, 0.0001F);
            BOOST_CHECK_SMALL(shift.z, 0.0001F);
        }
        BOOST_CHECK(v3d::editor::same(cube->placement(), placed));
        BOOST_CHECK(cube->translation() == placed.position);
        for (v3d::brep::Index face = 0; face < cube->geometry()->faceCount(); face++) {
            BOOST_CHECK_SMALL(glm::distance(cube->geometry()->face(face)->normal(), v3d::brep::newellNormal(*cube->geometry(), face)), 0.0001F);
        }

        // undo writes back what was stored, so the mesh is the same to the last bit
        history->undo();
        BOOST_CHECK(Snapshot(*cube->geometry()) == before);
        history->redo();
        BOOST_CHECK(Snapshot(*cube->geometry()) == after);
    }

    /**
     * The index of the cube's vertex at a corner.
     **/
    v3d::brep::Index corner(const glm::vec3& point) const {
        for (v3d::brep::Index id = 0; id < cube->geometry()->vertexCount(); id++) {
            if (glm::distance(cube->geometry()->vertex(id)->point(), point) < 0.0001F) {
                return id;
            }
        }
        return v3d::brep::INVALID_ID;
    }

    boost::shared_ptr<v3d::editor::Scene> scene;
    boost::shared_ptr<v3d::editor::Mesh> cube;
    boost::shared_ptr<v3d::editor::CommandStack> history;
    boost::shared_ptr<v3d::editor::TransformTool> transformTool;
};

BOOST_AUTO_TEST_CASE(transform_tool_vertex_test) {
    const Components components(v3d::editor::SelectMask::Vertex);
    const v3d::brep::Index vertex = components.corner(glm::vec3(0.5F, 0.5F, -0.5F));
    components.cube->geometry()->vertex(vertex)->selected(true);
    components.check({vertex});
}

BOOST_AUTO_TEST_CASE(transform_tool_edge_test) {
    const Components components(v3d::editor::SelectMask::Edge);
    const v3d::brep::Index from = components.corner(glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index to = components.corner(glm::vec3(0.5F, 0.5F, -0.5F));
    // the half edge that ends at one corner and follows one that ends at the other
    bool found = false;
    for (v3d::brep::Index id = 0; id < components.cube->geometry()->edgeCount() && !found; id++) {
        const v3d::brep::HalfEdge* edge = components.cube->geometry()->edge(id);
        if (edge->vertex() == from && components.cube->geometry()->edge(edge->next())->vertex() == to) {
            components.cube->geometry()->edge(edge->next())->selected(true);
            found = true;
        }
    }
    BOOST_REQUIRE(found);
    components.check({from, to});
}

BOOST_AUTO_TEST_CASE(transform_tool_face_test) {
    const Components components(v3d::editor::SelectMask::Face);
    std::vector<v3d::brep::Index> front;
    for (v3d::brep::Index face = 0; face < components.cube->geometry()->faceCount() && front.empty(); face++) {
        std::vector<v3d::brep::Index> corners;
        for (const v3d::brep::Index edge : v3d::brep::faceLoop(*components.cube->geometry(), face)) {
            const v3d::brep::Index vertex = components.cube->geometry()->edge(edge)->vertex();
            if (components.cube->geometry()->vertex(vertex)->point().z < -0.49F) {
                corners.push_back(vertex);
            }
        }
        if (corners.size() == 4) {
            components.cube->geometry()->face(face)->selected(true);
            front = corners;
        }
    }
    BOOST_REQUIRE_EQUAL(front.size(), 4U);
    components.check(front);
}

BOOST_AUTO_TEST_CASE(transform_tool_component_no_gesture_test) {
    const Components components(v3d::editor::SelectMask::Vertex);
    components.cube->geometry()->vertex(components.corner(glm::vec3(0.5F, 0.5F, -0.5F)))->selected(true);

    // a handle grabbed and let go without moving changed nothing
    const glm::vec2 handle = screen(*components.transformTool->view(), components.transformTool->target().pivot()) +
        glm::vec2(45.0F, 0.0F);
    components.transformTool->motion(handle);
    components.transformTool->button(1, true, handle);
    components.transformTool->button(1, false, handle);
    BOOST_CHECK_EQUAL(components.history->undoDepth(), 0U);

    // with no component selected there are no handles to grab
    components.cube->geometry()->deselectComponents();
    BOOST_CHECK(components.transformTool->target().empty());
}

BOOST_AUTO_TEST_CASE(transform_tool_mask_change_test) {
    const Components components(v3d::editor::SelectMask::Vertex);
    const v3d::brep::Index vertex = components.corner(glm::vec3(0.5F, 0.5F, -0.5F));
    components.cube->geometry()->vertex(vertex)->selected(true);
    const glm::vec3 start = components.cube->geometry()->vertex(vertex)->point();

    const glm::vec2 handle = screen(*components.transformTool->view(), components.transformTool->target().pivot()) +
        glm::vec2(45.0F, 0.0F);
    components.transformTool->motion(handle);
    components.transformTool->button(1, true, handle);
    components.transformTool->motion(handle + glm::vec2(100.0F, 0.0F));

    // a mask change ends the drag, and what it already moved is recorded
    components.transformTool->mask(v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(components.transformTool->dragging(), false);
    BOOST_REQUIRE_EQUAL(components.history->undoDepth(), 1U);
    components.history->undo();
    BOOST_CHECK(components.cube->geometry()->vertex(vertex)->point() == start);

    // and the handles now move the object
    BOOST_CHECK_EQUAL(components.transformTool->target().components(), false);
}

};  // namespace
