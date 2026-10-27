/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Primitives.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <optional>

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
 * A tool over a scene, in a front view, with no logger. A test has no use for the lines a
 * pick writes, and the tool accepts an empty logger.
 **/
boost::shared_ptr<v3d::editor::SelectTool> tool(const boost::shared_ptr<v3d::editor::Scene>& scene) {
    boost::shared_ptr<v3d::editor::SelectTool> select =
        boost::make_shared<v3d::editor::SelectTool>(scene, boost::shared_ptr<v3d::log::Logger>());
    select->view(frontView());
    return select;
}

/**
 * How many components of a mesh item are selected, across all three kinds.
 **/
std::size_t selectedComponents(const v3d::editor::Scene& scene, const boost::shared_ptr<v3d::editor::Mesh>& item) {
    std::size_t count = 0;
    for (const v3d::editor::Component kind : { v3d::editor::Component::Vertex, v3d::editor::Component::Edge,
        v3d::editor::Component::Face }) {
        count += scene.components().of(item->id(), kind).size();
    }
    return count;
}

BOOST_AUTO_TEST_CASE(select_tool_object_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Object);

    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // a miss in object mode clears the selection
    select->button(1, true, glm::vec2(10.0F, 10.0F));
    BOOST_CHECK_EQUAL(cube->selected(), false);

    // the release does nothing: a drag that ended somewhere else would pick there
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    select->button(1, false, glm::vec2(10.0F, 10.0F));
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // neither does any other button
    select->button(2, true, glm::vec2(10.0F, 10.0F));
    BOOST_CHECK_EQUAL(cube->selected(), true);
}

BOOST_AUTO_TEST_CASE(select_tool_one_object_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    first->translation(glm::vec3(-0.75F, 0.0F, 0.0F));
    scene->add(first);
    const boost::shared_ptr<v3d::editor::Mesh> second = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    second->translation(glm::vec3(0.75F, 0.0F, 0.0F));
    scene->add(second);

    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);

    // 0.75 world units either side of the middle, at two hundred pixels to the unit
    select->button(1, true, glm::vec2(50.0F, 200.0F));
    BOOST_CHECK_EQUAL(first->selected(), true);
    BOOST_CHECK_EQUAL(second->selected(), false);

    // one selection at a time: selecting the second deselects the first
    select->button(1, true, glm::vec2(350.0F, 200.0F));
    BOOST_CHECK_EQUAL(first->selected(), false);
    BOOST_CHECK_EQUAL(second->selected(), true);
}

/**
 * Two cubes side by side, 0.75 world units either side of the middle of the front view.
 **/
struct TwoCubes {
    TwoCubes() :
        scene(boost::make_shared<v3d::editor::Scene>()),
        left(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        right(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        leftCube(50.0F, 200.0F),
        rightCube(350.0F, 200.0F),
        nowhere(200.0F, 10.0F) {
        left->translation(glm::vec3(-0.75F, 0.0F, 0.0F));
        right->translation(glm::vec3(0.75F, 0.0F, 0.0F));
        scene->add(left);
        scene->add(right);
    }

    boost::shared_ptr<v3d::editor::Scene> scene;
    boost::shared_ptr<v3d::editor::Mesh> left;
    boost::shared_ptr<v3d::editor::Mesh> right;
    // at two hundred pixels to the unit, the middle of each cube's front face, and a point
    // on neither cube
    glm::vec2 leftCube;
    glm::vec2 rightCube;
    glm::vec2 nowhere;
};

BOOST_AUTO_TEST_CASE(select_tool_add_object_test) {
    const TwoCubes cubes;
    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(cubes.scene);

    select->button(1, true, cubes.leftCube);
    select->adding(true);
    select->button(1, true, cubes.rightCube);
    BOOST_CHECK_EQUAL(cubes.left->selected(), true);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);

    // a miss while adding keeps the set
    select->button(1, true, cubes.nowhere);
    BOOST_CHECK_EQUAL(cubes.scene->selection().size(), 2U);

    // a second click while adding takes the object away and leaves the other
    select->button(1, true, cubes.leftCube);
    BOOST_CHECK_EQUAL(cubes.left->selected(), false);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);

    // a plain click replaces the set
    select->button(1, true, cubes.leftCube);
    select->adding(false);
    select->button(1, true, cubes.rightCube);
    BOOST_CHECK_EQUAL(cubes.left->selected(), false);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);

    // and a plain miss clears it
    select->adding(true);
    select->button(1, true, cubes.leftCube);
    select->adding(false);
    select->button(1, true, cubes.nowhere);
    BOOST_CHECK(cubes.scene->selection().empty());
}

BOOST_AUTO_TEST_CASE(select_tool_add_component_test) {
    const TwoCubes cubes;
    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(cubes.scene);
    cubes.left->selected(true);
    cubes.right->selected(true);

    select->activate("face");
    select->button(1, true, cubes.leftCube);
    select->adding(true);
    select->button(1, true, cubes.rightCube);

    // the component selection spans both objects
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 1U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 1U);

    // a miss while adding keeps the components
    select->button(1, true, cubes.nowhere);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 1U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 1U);

    // a second click while adding takes the face away and leaves the other
    select->button(1, true, cubes.leftCube);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 0U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 1U);

    // a plain click replaces the components in every object
    select->adding(false);
    select->button(1, true, cubes.leftCube);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 1U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 0U);

    // a mask change clears the components in every object, and keeps both objects
    select->adding(true);
    select->button(1, true, cubes.rightCube);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 1U);
    select->activate("vertex");
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 0U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 0U);
    BOOST_CHECK_EQUAL(cubes.scene->selection().size(), 2U);
}

/**
 * Two half size cubes side by side, a quarter of the view either side of its middle. Each
 * covers fifty to a hundred and fifty pixels from the middle, and the band tests drag around
 * them from the empty space above and below.
 **/
struct TwoSmallCubes {
    TwoSmallCubes() :
        scene(boost::make_shared<v3d::editor::Scene>()),
        left(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        right(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())) {
        left->scale(glm::vec3(0.5F, 0.5F, 0.5F));
        right->scale(glm::vec3(0.5F, 0.5F, 0.5F));
        left->translation(glm::vec3(-0.5F, 0.0F, 0.0F));
        right->translation(glm::vec3(0.5F, 0.0F, 0.0F));
        scene->add(left);
        scene->add(right);
    }

    boost::shared_ptr<v3d::editor::Scene> scene;
    boost::shared_ptr<v3d::editor::Mesh> left;
    boost::shared_ptr<v3d::editor::Mesh> right;
};

/**
 * Press on one point, move to another and release there.
 **/
void drag(v3d::editor::SelectTool* select, const glm::vec2& from, const glm::vec2& to) {
    select->button(1, true, from);
    select->motion(to);
    select->button(1, false, to);
}

BOOST_AUTO_TEST_CASE(select_tool_band_object_test) {
    const TwoSmallCubes cubes;
    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(cubes.scene);

    // a band around both selects both
    drag(select.get(), glm::vec2(10.0F, 10.0F), glm::vec2(390.0F, 390.0F));
    BOOST_CHECK_EQUAL(cubes.left->selected(), true);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);

    // a band replaces the selection, and holds only what lies wholly inside it
    drag(select.get(), glm::vec2(390.0F, 10.0F), glm::vec2(190.0F, 390.0F));
    BOOST_CHECK_EQUAL(cubes.left->selected(), false);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);
    drag(select.get(), glm::vec2(390.0F, 10.0F), glm::vec2(100.0F, 390.0F));
    BOOST_CHECK_EQUAL(cubes.left->selected(), false);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);

    // while adding it adds, and leaves what was selected
    select->adding(true);
    drag(select.get(), glm::vec2(10.0F, 10.0F), glm::vec2(210.0F, 390.0F));
    BOOST_CHECK_EQUAL(cubes.left->selected(), true);
    BOOST_CHECK_EQUAL(cubes.right->selected(), true);

    // a band that starts on an object is not a band
    select->adding(false);
    drag(select.get(), glm::vec2(100.0F, 200.0F), glm::vec2(390.0F, 390.0F));
    BOOST_CHECK_EQUAL(cubes.left->selected(), true);
    BOOST_CHECK_EQUAL(cubes.right->selected(), false);
}

BOOST_AUTO_TEST_CASE(select_tool_band_component_test) {
    const TwoSmallCubes cubes;
    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(cubes.scene);
    cubes.left->selected(true);
    select->activate("face");

    // only the selected object's faces are found, and every one of them is inside
    drag(select.get(), glm::vec2(10.0F, 10.0F), glm::vec2(390.0F, 390.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 6U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 0U);
    BOOST_CHECK_EQUAL(cubes.right->selected(), false);

    // a band in a component mode replaces the components and keeps the objects
    cubes.right->selected(true);
    drag(select.get(), glm::vec2(390.0F, 10.0F), glm::vec2(210.0F, 390.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.left), 0U);
    BOOST_CHECK_EQUAL(selectedComponents(*cubes.scene, cubes.right), 6U);
    BOOST_CHECK_EQUAL(cubes.left->selected(), true);
}

BOOST_AUTO_TEST_CASE(select_tool_band_threshold_test) {
    const TwoSmallCubes cubes;
    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(cubes.scene);
    cubes.left->selected(true);

    // a press on nothing is held, and a press on something is not
    select->button(1, true, glm::vec2(100.0F, 200.0F));
    BOOST_CHECK_EQUAL(select->dragging(), false);
    select->button(1, false, glm::vec2(100.0F, 200.0F));
    select->button(1, true, glm::vec2(10.0F, 10.0F));
    BOOST_CHECK_EQUAL(select->dragging(), true);

    // the miss clears the selection on the press, and the press is not a band until the
    // cursor has moved past the tolerance
    BOOST_CHECK_EQUAL(cubes.left->selected(), false);
    const float within = v3d::editor::SelectTool::clickTolerance;
    select->motion(glm::vec2(10.0F + within, 10.0F + within));
    BOOST_CHECK(!select->band());
    select->motion(glm::vec2(10.0F + within + 1.0F, 10.0F));
    const std::optional<v3d::editor::SelectTool::Band> band = select->band();
    BOOST_REQUIRE(band.has_value());
    if (band) {
        BOOST_CHECK_EQUAL(band->corner.x, 10.0F);
        BOOST_CHECK_EQUAL(band->opposite.x, 10.0F + within + 1.0F);
    }

    // a shaky click is a click
    select->motion(glm::vec2(12.0F, 12.0F));
    select->button(1, false, glm::vec2(12.0F, 12.0F));
    BOOST_CHECK_EQUAL(select->dragging(), false);
    BOOST_CHECK(cubes.scene->selection().empty());

    // and a release with no press held does nothing, even far from where the last one was
    select->motion(glm::vec2(390.0F, 390.0F));
    select->button(1, false, glm::vec2(390.0F, 390.0F));
    BOOST_CHECK(cubes.scene->selection().empty());
}

BOOST_AUTO_TEST_CASE(select_tool_component_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);
    select->activate("face");
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Face);

    // nothing is selected, so there is no object to pick a component of
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, cube), 0U);

    select->activate("object");
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(cube->selected(), true);

    select->activate("face");
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, cube), 1U);
    // the object stays selected while its components are being worked on
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // clicking the same face again deselects it
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, cube), 0U);

    // a miss clears the components without losing the object
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    select->button(1, true, glm::vec2(10.0F, 10.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, cube), 0U);
    BOOST_CHECK_EQUAL(cube->selected(), true);
}

BOOST_AUTO_TEST_CASE(select_tool_mask_change_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    scene->add(cube);

    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    select->activate("face");
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(selectedComponents(*scene, cube), 1U);

    // changing the mask clears the components: a face selection means nothing to an
    // operation working on edges, and leaving it set would let two kinds be selected at once
    select->activate("edge");
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Edge);
    BOOST_CHECK_EQUAL(selectedComponents(*scene, cube), 0U);
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // a name that is not a mask leaves the tool alone
    select->activate("curve");
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Edge);
}

BOOST_AUTO_TEST_CASE(select_tool_no_view_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    scene->add(cube);

    // the cursor is over no view until it has moved into one, and a click then picks
    // nothing rather than dereferencing what it has not been given
    v3d::editor::SelectTool select(scene, boost::shared_ptr<v3d::log::Logger>());
    select.button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK_EQUAL(cube->selected(), false);
    BOOST_CHECK_EQUAL(select.hit().valid, false);
}

BOOST_AUTO_TEST_CASE(select_mask_kinds_test) {
    // every mask's name reads back as that mask
    for (const v3d::editor::SelectMask mask : { v3d::editor::SelectMask::Object, v3d::editor::SelectMask::Vertex,
        v3d::editor::SelectMask::Edge, v3d::editor::SelectMask::Face, v3d::editor::SelectMask::Mesh,
        v3d::editor::SelectMask::Light, v3d::editor::SelectMask::Camera }) {
        v3d::editor::SelectMask read = v3d::editor::SelectMask::Face;
        BOOST_CHECK(v3d::editor::selectMask(v3d::editor::selectMaskName(mask), &read));
        BOOST_CHECK(read == mask);
    }

    // the whole item masks take their own kind, and object takes every kind
    BOOST_CHECK(v3d::editor::selectsItems(v3d::editor::SelectMask::Light));
    BOOST_CHECK(!v3d::editor::selectsItems(v3d::editor::SelectMask::Edge));
    BOOST_CHECK(v3d::editor::takes(v3d::editor::SelectMask::Object, v3d::editor::Item::Kind::Camera));
    BOOST_CHECK(v3d::editor::takes(v3d::editor::SelectMask::Camera, v3d::editor::Item::Kind::Camera));
    BOOST_CHECK(!v3d::editor::takes(v3d::editor::SelectMask::Camera, v3d::editor::Item::Kind::Light));
    BOOST_CHECK(!v3d::editor::takes(v3d::editor::SelectMask::Mesh, v3d::editor::Item::Kind::Light));
    BOOST_CHECK(!v3d::editor::takes(v3d::editor::SelectMask::Face, v3d::editor::Item::Kind::Mesh));
    BOOST_CHECK(!v3d::editor::selectMask("curve", nullptr));
}

BOOST_AUTO_TEST_CASE(select_tool_light_mask_test) {
    const boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    light->translation(glm::vec3(0.0F, 0.0F, -0.5F));
    scene->add(cube);
    scene->add(light);
    const boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);

    // under the light mask a click selects the light whole, through the cube in front of the
    // view's middle, which that mask does not take
    select->activate("light");
    select->button(1, true, glm::vec2(200.0F, 200.0F));
    BOOST_CHECK(light->selected());
    BOOST_CHECK(!cube->selected());

    // and a miss clears it, as a miss under the object mask does
    select->button(1, true, glm::vec2(10.0F, 10.0F));
    BOOST_CHECK(!light->selected());
}

};  // namespace
