/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/RIBReader.h>
#include <talyn/libtalyn/RIBHandler.h>

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>

namespace {

bool read(const std::string & source, v3d::talyn::RIBHandler * handler) {
    v3d::render::offline::RIBReader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(source);
    return reader.read(stream, handler);
}

};  // namespace

/**
 * Format is what allocates the framebuffer, which is the one request talyn's own reader ever
 * implemented.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_format_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "##RenderMan RIB-Structure 1.1\n"
        "version 3.03\n"
        "Format 32 16 1\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE(rc->framebuffer());
    BOOST_CHECK_EQUAL(rc->framebuffer()->width(), 32u);
    BOOST_CHECK_EQUAL(rc->framebuffer()->height(), 16u);
    BOOST_CHECK_EQUAL(handler.error(), "");
}

BOOST_AUTO_TEST_CASE(talyn_ribhandler_missing_file_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::RIBReader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_CHECK(!reader.read("data/no-such-scene.rib", &handler));
}

BOOST_AUTO_TEST_CASE(talyn_ribhandler_fixture_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::RIBReader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read("data/format.rib", &handler));
    BOOST_REQUIRE(rc->framebuffer());
    BOOST_CHECK_EQUAL(rc->framebuffer()->width(), 32u);
}

/**
 * A polygon is fanned into triangles, and its points go through the current transformation on
 * the way in - a scene holds triangles in world space.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_polygon_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "WorldBegin\n"
        "Translate 1 2 3\n"
        "Polygon \"P\" [0 0 0  1 0 0  1 1 0  0 1 0]\n"
        "WorldEnd\n", &handler));

    // a quad fans into two triangles
    BOOST_REQUIRE_EQUAL(rc->scene().triangles().size(), 2u);
    const v3d::talyn::Triangle & first = rc->scene().triangles()[0];
    BOOST_CHECK_EQUAL(first.a().x, 1.0f);
    BOOST_CHECK_EQUAL(first.a().y, 2.0f);
    BOOST_CHECK_EQUAL(first.a().z, 3.0f);
    BOOST_CHECK_EQUAL(first.b().x, 2.0f);
}

BOOST_AUTO_TEST_CASE(talyn_ribhandler_points_polygons_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "WorldBegin\n"
        "PointsPolygons [4 3] [0 1 2 3  0 2 3] \"P\" [0 0 0  1 0 0  1 1 0  0 1 0]\n"
        "WorldEnd\n", &handler));

    // a quad is two triangles and a triangle is one
    BOOST_CHECK_EQUAL(rc->scene().triangles().size(), 3u);
}

/**
 * Color is the flat colour a triangle carries, and it pushes and pops with an attribute block.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_color_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "WorldBegin\n"
        "AttributeBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [0 0 1  1 0 1  0 1 1]\n"
        "AttributeEnd\n"
        "Polygon \"P\" [0 0 1  1 0 1  0 1 1]\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE_EQUAL(rc->scene().triangles().size(), 2u);
    BOOST_CHECK_EQUAL(rc->scene().triangles()[0].colour().r, 1.0f);
    BOOST_CHECK_EQUAL(rc->scene().triangles()[0].colour().g, 0.0f);
    // the block restored the default white
    BOOST_CHECK_EQUAL(rc->scene().triangles()[1].colour().g, 1.0f);
}

/**
 * TransformEnd restores what TransformBegin saved.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_transform_block_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "WorldBegin\n"
        "TransformBegin\n"
        "Translate 10 10 10\n"
        "TransformEnd\n"
        "Polygon \"P\" [0 0 1  1 0 1  0 1 1]\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE_EQUAL(rc->scene().triangles().size(), 1u);
    BOOST_CHECK_EQUAL(rc->scene().triangles()[0].a().x, 0.0f);
    BOOST_CHECK_EQUAL(rc->scene().triangles()[0].a().z, 1.0f);
}

/**
 * The world to camera transformation is what a scene sets between Projection and WorldBegin,
 * and the camera is built from it there: the eye is where the matrix puts the world origin
 * back, and the orientation is its rotation.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_camera_placement_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "Projection \"perspective\" \"fov\" 45\n"
        "Clipping 1 100\n"
        "Transform [1 0 0 0  0 1 0 0  0 0 1 0  0 0 4 1]\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.error(), "");
    const v3d::type::camera::Profile & profile = rc->scene().camera().profile();
    BOOST_CHECK_CLOSE(profile.eye().z, -4.0f, 0.01f);
    BOOST_CHECK_EQUAL(profile.eye().x, 0.0f);
    BOOST_CHECK(!profile.orthographic());
    BOOST_CHECK_CLOSE(profile.clipping().x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.clipping().y, 100.0f, 0.01f);
}

/**
 * An orthographic scene takes its aperture from the screen window, which by default follows
 * the frame aspect.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_orthographic_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.error(), "");
    const v3d::type::camera::Profile & profile = rc->scene().camera().profile();
    BOOST_CHECK(profile.orthographic());
    BOOST_CHECK_CLOSE(profile.orthoZoom(), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.pixelAspect(), 4.0f / 3.0f, 0.01f);
}

/**
 * What a Profile cannot hold is refused rather than rendered as something else. An off
 * centre window has no field of view to state, and a matrix that reverses handedness is not a
 * rotation and a translation - the standard's own example camera is one of those.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_unsupported_camera_test) {
    auto offCentre = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler first(offCentre);
    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "ScreenWindow 0 2 -1 1\n"
        "WorldBegin\n"
        "WorldEnd\n", &first));
    BOOST_CHECK(first.error().contains("off centre"));

    auto mirrored = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler second(mirrored);
    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "Transform [-1 0 0 0  0 1 0 0  0 0 1 0  0 0 4 1]\n"
        "WorldBegin\n"
        "WorldEnd\n", &second));
    BOOST_CHECK(second.error().contains("rotation and a translation"));

    auto scaled = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler third(scaled);
    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "Transform [2 0 0 0  0 2 0 0  0 0 2 0  0 0 4 1]\n"
        "WorldBegin\n"
        "WorldEnd\n", &third));
    BOOST_CHECK(third.error().contains("rotation and a translation"));
}

/**
 * A scene a file describes, rendered: the triangle a file names covers the pixels it should
 * and the background covers the rest.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_renders_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 0.001 100\n"
        "Transform [1 0 0 0  0 1 0 0  0 0 1 0  0 0 1 1]\n"
        "WorldBegin\n"
        "Color [0.9 0.2 0.2]\n"
        "Polygon \"P\" [-0.8 -0.6 2  0.8 -0.6 2  0 0.7 2]\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE_EQUAL(handler.error(), "");
    rc->render();

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc->framebuffer();
    // the middle of the triangle
    BOOST_CHECK_CLOSE(planes->value(0, 32, 28), 0.9f, 0.01f);
    // and a corner of the frame, which nothing covers
    BOOST_CHECK_EQUAL(planes->value(0, 1, 1), 0.0f);
}

/**
 * A polygon that says nothing about its normals takes its own plane, in world space.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_face_normal_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "Projection \"orthographic\"\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 0 0  1 0 0  0 1 0]\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE_EQUAL(rc->scene().triangles().size(), 1u);
    const v3d::talyn::Triangle & triangle = rc->scene().triangles()[0];
    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangle.shadingNormal(0.25f, 0.25f) == glm::vec3(0.0f, 0.0f, 1.0f)));
}

/**
 * A varying "N" is the shading normal and overrides the plane. Ng stays the plane, so a hit
 * has both.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_normal_override_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "Projection \"orthographic\"\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 0 0  1 0 0  0 1 0] \"N\" [0 1 0  0 1 0  0 1 0]\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE_EQUAL(rc->scene().triangles().size(), 1u);
    const v3d::talyn::Triangle & triangle = rc->scene().triangles()[0];
    BOOST_TEST((triangle.shadingNormal(0.25f, 0.25f) == glm::vec3(0.0f, 1.0f, 0.0f)));
    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
}

/**
 * A normal transforms by the inverse transpose of the current transformation, not by the matrix
 * that moves the points. The two agree under a rotation and a uniform scale, so only a scene
 * that scales one axis tells them apart - and there the normal leans the opposite way to the
 * points.
 **/
BOOST_AUTO_TEST_CASE(talyn_ribhandler_normal_inverse_transpose_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);

    BOOST_REQUIRE(read(
        "Format 32 16 1\n"
        "Projection \"orthographic\"\n"
        "WorldBegin\n"
        "Scale 1 2 1\n"
        "Polygon \"P\" [1 0 0  0 1 0  0 1 1] \"N\" [1 1 0  1 1 0  1 1 0]\n"
        "WorldEnd\n", &handler));

    BOOST_REQUIRE_EQUAL(rc->scene().triangles().size(), 1u);
    const v3d::talyn::Triangle & triangle = rc->scene().triangles()[0];

    // (1, 1, 0) under the inverse transpose of a scale of two in y has its y halved
    const glm::vec3 expected = glm::normalize(glm::vec3(1.0f, 0.5f, 0.0f));
    const glm::vec3 shading = triangle.shadingNormal(0.25f, 0.25f);
    BOOST_TEST(shading.x == expected.x, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(shading.y == expected.y, boost::test_tools::tolerance(0.0001f));

    // the "N" given was the polygon's own plane, so the transformed one is the plane the
    // transformed points lie in - which the geometric normal, built in world space, agrees with
    BOOST_TEST(triangle.geometricNormal().x == expected.x, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(triangle.geometricNormal().y == expected.y, boost::test_tools::tolerance(0.0001f));
}
