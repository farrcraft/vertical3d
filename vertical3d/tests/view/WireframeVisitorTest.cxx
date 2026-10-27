/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <api/curve/Curves.h>
#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/Glyphs.h>
#include <vertical3d/src/view/WireframeVisitor.h>

#include <cmath>
#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A unit quad in the z = 0 plane. One face, so none of its four edges has a pair.
 **/
boost::shared_ptr<v3d::editor::Mesh> quad() {
    boost::shared_ptr<v3d::editor::Mesh> mesh = boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
    std::vector<glm::vec3> points;
    points.push_back(glm::vec3(0.0F, 0.0F, 0.0F));
    points.push_back(glm::vec3(1.0F, 0.0F, 0.0F));
    points.push_back(glm::vec3(1.0F, 1.0F, 0.0F));
    points.push_back(glm::vec3(0.0F, 1.0F, 0.0F));
    mesh->geometry()->addFace(points, glm::vec3(0.0F, 0.0F, 1.0F));
    return mesh;
}

/**
 * Where an item with no parent is, as the scene's evaluation would place it.
 **/
template <typename T>
v3d::editor::Placed at(const boost::shared_ptr<T>& item) {
    v3d::editor::Placed placed;
    if (item) {
        placed.local = item->matrix();
        placed.world = placed.local;
    }
    return placed;
}

BOOST_AUTO_TEST_CASE(wireframe_single_face_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    wireframe.visit(quad(), at(quad()));

    // four edges, two vertices each, and the loop closes
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 8U);
}

BOOST_AUTO_TEST_CASE(wireframe_pairs_drawn_once_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    wireframe.visit(cube, at(cube));

    // 24 half edges paired into the cube's 12 edges. A half edge and its pair are one
    // segment, and drawing both would double every line
    BOOST_CHECK_EQUAL(cube->geometry()->edgeCount(), 24U);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 24U);
}

BOOST_AUTO_TEST_CASE(wireframe_transform_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    const boost::shared_ptr<v3d::editor::Mesh> mesh = quad();
    mesh->translation(glm::vec3(10.0F, 0.0F, 0.0F));
    wireframe.visit(mesh, at(mesh));

    // the mesh is drawn through its own transform, so its geometry stays about its origin
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8U);
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK(vertex.position.x >= 9.9F);
        BOOST_CHECK(vertex.position.x <= 11.1F);
    }

    // the transform is popped, so the next mesh does not inherit it
    BOOST_CHECK_SMALL(canvas.transform()[3][0], 0.0001F);
}

BOOST_AUTO_TEST_CASE(wireframe_selection_colour_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    const boost::shared_ptr<v3d::editor::Mesh> mesh = quad();
    wireframe.visit(mesh, at(mesh));
    const glm::vec4 unselected = canvas.vertices()[0].colour;

    canvas.clear();
    mesh->selected(true);
    wireframe.visit(mesh, at(mesh));
    const glm::vec4 object = canvas.vertices()[0].colour;
    BOOST_CHECK((object != unselected));

    canvas.clear();
    mesh->selected(false);
    v3d::editor::Components picked;
    picked.select(mesh->id(), v3d::editor::Component::Edge, 0, true);
    v3d::editor::WireframeVisitor selecting(&canvas, v3d::editor::WireframeVisitor::Shown{}, &picked);
    selecting.visit(mesh, at(mesh));

    // one edge selected out of four, and the other three keep the unselected colour
    unsigned int highlighted = 0;
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
        if (vertex.colour != unselected) {
            highlighted++;
        }
    }
    BOOST_CHECK_EQUAL(highlighted, 2U);
}

/**
 * A selected face is outlined on all four of its edges, including those the neighbouring face
 * draws.
 **/
BOOST_AUTO_TEST_CASE(wireframe_selected_face_is_outlined_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::Components picked;
    v3d::editor::WireframeVisitor wireframe(&canvas, v3d::editor::WireframeVisitor::Shown{}, &picked);

    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    wireframe.visit(cube, at(cube));
    const glm::vec4 unselected = canvas.vertices()[0].colour;

    for (v3d::brep::Index face = 0; face < cube->geometry()->faceCount(); face++) {
        canvas.clear();
        picked.select(cube->id(), v3d::editor::Component::Face, face, true);
        wireframe.visit(cube, at(cube));
        picked.select(cube->id(), v3d::editor::Component::Face, face, false);

        unsigned int highlighted = 0;
        for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
            if (vertex.colour != unselected) {
                highlighted++;
            }
        }
        // four edges of two vertices each
        BOOST_CHECK_EQUAL(highlighted, 8U);
    }
}

BOOST_AUTO_TEST_CASE(wireframe_scene_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    v3d::editor::Scene scene;
    scene.add(quad());
    scene.add(quad());
    scene.accept(&wireframe);

    BOOST_CHECK_EQUAL(canvas.vertices().size(), 16U);
}

BOOST_AUTO_TEST_CASE(wireframe_glyph_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas);

    // a light is drawn as its glyph, through its translation
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    light->translation(glm::vec3(5.0F, 0.0F, 0.0F));
    wireframe.visit(light, at(light), light->emission());
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 6U);
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK(vertex.position.x >= 4.7F);
        BOOST_CHECK(vertex.position.x <= 5.3F);
    }
    const glm::vec4 unselected = canvas.vertices()[0].colour;

    // a selected one in the selection's colour, which is the colour of a selected mesh
    canvas.clear();
    light->selected(true);
    wireframe.visit(light, at(light), light->emission());
    const glm::vec4 selected = canvas.vertices()[0].colour;
    BOOST_CHECK((selected != unselected));
    canvas.clear();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = quad();
    mesh->selected(true);
    wireframe.visit(mesh, at(mesh));
    BOOST_CHECK((canvas.vertices()[0].colour == selected));

    // a camera is drawn as its glyph too, and the transform is popped after it
    canvas.clear();
    const boost::shared_ptr<v3d::editor::Camera> camera = boost::make_shared<v3d::editor::Camera>();
    wireframe.visit(camera, at(camera), camera->lens());
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 22U);
    BOOST_CHECK_SMALL(canvas.transform()[3][0], 0.0001F);
}

BOOST_AUTO_TEST_CASE(wireframe_shown_kinds_test) {
    v3d::editor::Scene scene;
    scene.add(quad());
    scene.add(boost::make_shared<v3d::editor::Camera>());
    scene.add(boost::make_shared<v3d::editor::Light>());

    // a quad of four segments, a camera of eleven, a point light of three
    const auto drawn = [&scene](const v3d::editor::WireframeVisitor::Shown& shown) {
        v3d::render::realtime::canvas::Line canvas;
        v3d::editor::WireframeVisitor wireframe(&canvas, shown);
        scene.accept(&wireframe);
        return canvas.vertices().size() / 2;
    };
    BOOST_CHECK_EQUAL(drawn({}), 18U);
    BOOST_CHECK_EQUAL(drawn({ .meshes = false }), 14U);
    BOOST_CHECK_EQUAL(drawn({ .cameras = false }), 7U);
    BOOST_CHECK_EQUAL(drawn({ .lights = false }), 15U);
    BOOST_CHECK_EQUAL(drawn({ .meshes = false, .cameras = false, .lights = false }), 0U);
}

BOOST_AUTO_TEST_CASE(wireframe_no_canvas_test) {
    v3d::editor::WireframeVisitor wireframe(nullptr);

    // a view that is not drawing meshes hands nothing to draw into
    wireframe.visit(quad(), at(quad()));
    wireframe.visit(boost::shared_ptr<v3d::editor::Mesh>(), at(boost::shared_ptr<v3d::editor::Mesh>()));
}

BOOST_AUTO_TEST_CASE(wireframe_beneath_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = quad();
    group->translation(glm::vec3(5.0F, 0.0F, 0.0F));
    group->scale(glm::vec3(4.0F));
    scene.add(group);
    scene.add(mesh);
    scene.parent(mesh->id(), group->id());

    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::WireframeVisitor wireframe(&canvas, { .meshes = true, .cameras = false, .lights = false });
    scene.accept(&wireframe);

    // the group is a box of twelve segments, drawn first and at its own size whatever its
    // scale, and the quad after it, where the group puts it
    const std::vector<v3d::render::realtime::canvas::Line::Vertex>& drawn = canvas.vertices();
    BOOST_REQUIRE_EQUAL(drawn.size(), 32U);
    for (std::size_t index = 0; index < 24; index++) {
        BOOST_CHECK_SMALL(std::abs(drawn[index].position.x - 5.0F) - v3d::editor::GROUP_REACH, 0.0001F);
    }
    for (std::size_t index = 24; index < drawn.size(); index++) {
        BOOST_CHECK(drawn[index].position.x >= 5.0F - 0.0001F);
        BOOST_CHECK(drawn[index].position.x <= 9.0F + 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(wireframe_curve_test) {
    v3d::render::realtime::canvas::Line canvas;
    // a curve is drawn whichever other kinds are hidden
    v3d::editor::WireframeVisitor wireframe(&canvas, { .meshes = false, .cameras = false, .lights = false });

    // a Catmull-Rom curve of four points is three segments, each drawn in as many lines as the
    // geometry's resolution, through the item's world matrix
    const boost::shared_ptr<v3d::editor::Curve> curve = v3d::editor::create_curve(v3d::editor::CurveType::CatmullRom);
    curve->translation(glm::vec3(10.0F, 0.0F, 0.0F));
    wireframe.visit(curve, at(curve));
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 2U * 3U * v3d::curve::Curves::DEFAULT_RESOLUTION);
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK(vertex.position.x >= 8.5F);
        BOOST_CHECK(vertex.position.x <= 11.5F);
    }
    BOOST_CHECK_SMALL(canvas.transform()[3][0], 0.0001F);
    const glm::vec4 unselected = canvas.vertices()[0].colour;

    canvas.clear();
    curve->selected(true);
    wireframe.visit(curve, at(curve));
    BOOST_CHECK((canvas.vertices()[0].colour != unselected));
}

};  // namespace
