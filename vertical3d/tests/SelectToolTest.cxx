/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../src/CreatePoly.h"
#include "../src/MeshTopology.h"
#include "../src/Scene.h"
#include "../src/SelectTool.h"
#include "../src/ViewPort.h"

namespace {

    /**
     * The same square front view the picker's own tests use: a world unit is two hundred
     * pixels and the middle of the view is the middle of a unit primitive.
     **/
    boost::shared_ptr<v3d::editor::ViewPort> frontView() {
        v3d::type::CameraProfile profile("front");
        boost::shared_ptr<v3d::editor::ViewPort> view = boost::make_shared<v3d::editor::ViewPort>("front", profile);
        view->resize(glm::vec4(0.0f, 0.0f, 400.0f, 400.0f));
        return view;
    }

    /**
     * A tool over a scene, in a front view, with no logger - a test has no use for the
     * lines a pick writes, and the tool takes an empty one.
     **/
    boost::shared_ptr<v3d::editor::SelectTool> tool(const boost::shared_ptr<v3d::editor::Scene>& scene) {
        boost::shared_ptr<v3d::editor::SelectTool> select =
            boost::make_shared<v3d::editor::SelectTool>(scene, boost::shared_ptr<v3d::log::Logger>());
        select->view(frontView());
        return select;
    }

    /**
     * How many components of a mesh are selected, across all three kinds.
     **/
    std::size_t selectedComponents(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
        std::size_t count = 0;
        for (std::size_t index = 0; index < mesh->vertexCount(); index++) {
            count += mesh->vertex(static_cast<unsigned int>(index))->selected() ? 1 : 0;
        }
        for (std::size_t index = 0; index < mesh->edgeCount(); index++) {
            count += mesh->edge(static_cast<unsigned int>(index))->selected() ? 1 : 0;
        }
        for (std::size_t index = 0; index < mesh->faceCount(); index++) {
            count += mesh->face(static_cast<unsigned int>(index))->selected() ? 1 : 0;
        }
        return count;
    }

};  // namespace

BOOST_AUTO_TEST_CASE(select_tool_object_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    scene->add(cube);

    boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Object);

    select->button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // a miss in object mode clears the selection, which is how a modeller deselects
    select->button(1, true, glm::vec2(10.0f, 10.0f));
    BOOST_CHECK_EQUAL(cube->selected(), false);

    // the release does nothing: a drag that ended somewhere else would pick there
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    select->button(1, false, glm::vec2(10.0f, 10.0f));
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // neither does any other button
    select->button(2, true, glm::vec2(10.0f, 10.0f));
    BOOST_CHECK_EQUAL(cube->selected(), true);
}

BOOST_AUTO_TEST_CASE(select_tool_one_object_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> first = v3d::editor::create_poly_cube();
    first->translation(glm::vec3(-0.75f, 0.0f, 0.0f));
    scene->add(first);
    boost::shared_ptr<v3d::brep::BRep> second = v3d::editor::create_poly_cube();
    second->translation(glm::vec3(0.75f, 0.0f, 0.0f));
    scene->add(second);

    boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);

    // 0.75 world units either side of the middle, at two hundred pixels to the unit
    select->button(1, true, glm::vec2(50.0f, 200.0f));
    BOOST_CHECK_EQUAL(first->selected(), true);
    BOOST_CHECK_EQUAL(second->selected(), false);

    // one selection at a time: selecting the second deselects the first
    select->button(1, true, glm::vec2(350.0f, 200.0f));
    BOOST_CHECK_EQUAL(first->selected(), false);
    BOOST_CHECK_EQUAL(second->selected(), true);
}

BOOST_AUTO_TEST_CASE(select_tool_component_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    scene->add(cube);

    boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);
    select->activate("face");
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Face);

    // nothing is selected, so there is no object to pick a component of
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(selectedComponents(cube), 0u);

    select->activate("object");
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(cube->selected(), true);

    select->activate("face");
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(selectedComponents(cube), 1u);
    // the object stays selected while its components are being worked on
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // clicking the same face again deselects it
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(selectedComponents(cube), 0u);

    // a miss clears the components without losing the object
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    select->button(1, true, glm::vec2(10.0f, 10.0f));
    BOOST_CHECK_EQUAL(selectedComponents(cube), 0u);
    BOOST_CHECK_EQUAL(cube->selected(), true);
}

BOOST_AUTO_TEST_CASE(select_tool_mask_change_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    scene->add(cube);

    boost::shared_ptr<v3d::editor::SelectTool> select = tool(scene);
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    select->activate("face");
    select->button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(selectedComponents(cube), 1u);

    // changing the mask clears the components: a face selection means nothing to an
    // operation working on edges, and leaving it set is what would let two kinds be
    // selected at once
    select->activate("edge");
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Edge);
    BOOST_CHECK_EQUAL(selectedComponents(cube), 0u);
    BOOST_CHECK_EQUAL(cube->selected(), true);

    // a name that is not a mask leaves the tool alone
    select->activate("curve");
    BOOST_CHECK(select->mask() == v3d::editor::SelectMask::Edge);
}

BOOST_AUTO_TEST_CASE(select_tool_no_view_test) {
    boost::shared_ptr<v3d::editor::Scene> scene = boost::make_shared<v3d::editor::Scene>();
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    scene->add(cube);

    // the cursor is over no view until it has moved into one, and a click then picks
    // nothing rather than dereferencing what it has not been given
    v3d::editor::SelectTool select(scene, boost::shared_ptr<v3d::log::Logger>());
    select.button(1, true, glm::vec2(200.0f, 200.0f));
    BOOST_CHECK_EQUAL(cube->selected(), false);
    BOOST_CHECK_EQUAL(select.hit().valid, false);
}
