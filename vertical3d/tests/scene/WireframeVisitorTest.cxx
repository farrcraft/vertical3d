/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../../src/scene/CreatePoly.h"
#include "../../src/scene/Scene.h"
#include "../../src/scene/WireframeVisitor.h"

namespace {

/**
 * A unit quad in the z = 0 plane. One face, so none of its four edges has a pair.
 **/
boost::shared_ptr<v3d::brep::BRep> quad() {
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();
    std::vector<glm::vec3> points;
    points.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    points.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    points.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
    points.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    mesh->addFace(points, glm::vec3(0.0f, 0.0f, 1.0f));
    return mesh;
}

};  // namespace

BOOST_AUTO_TEST_CASE(wireframe_single_face_test) {
    v3d::render::realtime::LineCanvas canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    wireframe.visit(quad());

    // four edges, two vertices each, and the loop closes
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 8u);
}

BOOST_AUTO_TEST_CASE(wireframe_pairs_drawn_once_test) {
    v3d::render::realtime::LineCanvas canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    wireframe.visit(cube);

    // 24 half edges paired into the cube's 12 edges - a half edge and its pair are one
    // segment, and drawing both would double every line the modeller sees
    BOOST_CHECK_EQUAL(cube->edgeCount(), 24u);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 24u);
}

BOOST_AUTO_TEST_CASE(wireframe_transform_test) {
    v3d::render::realtime::LineCanvas canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    boost::shared_ptr<v3d::brep::BRep> mesh = quad();
    mesh->translation(glm::vec3(10.0f, 0.0f, 0.0f));
    wireframe.visit(mesh);

    // the mesh is drawn through its own transform, so its geometry stays about its origin
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8u);
    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK(vertex.position.x >= 9.9f);
        BOOST_CHECK(vertex.position.x <= 11.1f);
    }

    // and the transform is popped, so the next mesh does not inherit it
    BOOST_CHECK_SMALL(canvas.transform()[3][0], 0.0001f);
}

BOOST_AUTO_TEST_CASE(wireframe_selection_colour_test) {
    v3d::render::realtime::LineCanvas canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    boost::shared_ptr<v3d::brep::BRep> mesh = quad();
    wireframe.visit(mesh);
    const glm::vec4 unselected = canvas.vertices()[0].colour;

    canvas.clear();
    mesh->selected(true);
    wireframe.visit(mesh);
    const glm::vec4 object = canvas.vertices()[0].colour;
    BOOST_CHECK((object != unselected));

    canvas.clear();
    mesh->selected(false);
    mesh->edge(0)->selected(true);
    wireframe.visit(mesh);

    // one edge selected out of four, and the other three keep the unselected colour
    unsigned int highlighted = 0;
    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        if (vertex.colour != unselected) {
            highlighted++;
        }
    }
    BOOST_CHECK_EQUAL(highlighted, 2u);
}

BOOST_AUTO_TEST_CASE(wireframe_scene_test) {
    v3d::render::realtime::LineCanvas canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    v3d::editor::Scene scene;
    scene.add(quad());
    scene.add(quad());
    scene.accept(&wireframe);

    BOOST_CHECK_EQUAL(canvas.vertices().size(), 16u);
}

BOOST_AUTO_TEST_CASE(wireframe_no_canvas_test) {
    v3d::editor::WireframeVisitor wireframe(nullptr);

    // a view that is not drawing meshes hands nothing to draw into
    wireframe.visit(quad());
    wireframe.visit(boost::shared_ptr<v3d::brep::BRep>());
}
