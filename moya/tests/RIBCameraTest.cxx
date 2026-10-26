/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Triangle.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

#include "Handling.h"

namespace v3d::moya::handling {
namespace {

/**
 * A scene read through the handler sets the render context's image size and builds its frame
 * buffer.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_camera_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().imageWidth(), 64U);
    BOOST_CHECK_EQUAL(handler.context().imageHeight(), 48U);
    BOOST_REQUIRE(handler.context().picture());
    BOOST_CHECK_EQUAL(handler.context().picture()->width(), 64U);
}

/**
 * The sampling requests reach the render context, and a scene that names none of them is
 * sampled at the RI defaults.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_sampling_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "PixelSamples 4 4\n"
        "DepthOfField 8 0.1 3\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().sampling().samples.x, 4U);
    BOOST_CHECK_EQUAL(handler.context().sampling().samples.y, 4U);
    BOOST_CHECK_EQUAL(handler.context().sampling().fstop, 8.0F);
    BOOST_CHECK_CLOSE(handler.context().sampling().focalLength, 0.1F, 1.0e-4F);
    BOOST_CHECK_EQUAL(handler.context().sampling().focalDistance, 3.0F);

    v3d::moya::RIBHandler silent;
    BOOST_REQUIRE(read("Format 64 48 1\nWorldBegin\nWorldEnd\n", &silent));
    BOOST_CHECK_EQUAL(silent.context().sampling().samples.x, 2U);
    BOOST_CHECK_EQUAL(silent.context().sampling().samples.y, 2U);
    BOOST_CHECK(silent.context().sampling().filter == v3d::render::offline::Filter::Gaussian);
    BOOST_CHECK_EQUAL(silent.context().sampling().width.x, 2.0F);
    BOOST_CHECK(silent.context().sampling().pinhole());
}

/**
 * A scene places its camera with a matrix that translates, as the standard's own example
 * does. The world to camera transformation applies as it stands, with no transpose or inverse.
 * Transposing and inverting a matrix gives back the same matrix only when it is a rotation.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_camera_transform_test) {
    v3d::moya::RIBHandler handler;

    // the camera sits four units back along -z, so a quad at the world origin is four units
    // in front of it and inside the [1, 100] clip range
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "Transform [1 0 0 0  0 1 0 0  0 0 1 0  0 0 4 1]\n"
        "WorldBegin\n"
        "Polygon \"P\" [-0.2 -0.2 0  0.2 -0.2 0  0.2 0.2 0  -0.2 0.2 0]\n"
        "WorldEnd\n", &handler));

    // the quad survived the hither-yon cull. At a depth of zero it would have been culled
    BOOST_CHECK_EQUAL(handler.context().bucketed(), 1U);
}

/**
 * A perspective projection makes the nearer of two equal quads the larger one. An identity
 * projection would make them the same size.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_perspective_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"perspective\" \"fov\" 45\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.4 -0.4 4  0.4 -0.4 4  0.4 0.4 4  -0.4 0.4 4]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();

    unsigned int covered = 0;
    for (unsigned int row = 0; row < 48; row++) {
        for (unsigned int column = 0; column < 64; column++) {
            if (planes->value(v3d::moya::Planes::RED, column, row) > 0.5F) {
                covered++;
            }
        }
    }
    BOOST_CHECK_GT(covered, 0U);

    // the same quad twice as far away covers a quarter of the pixels under a perspective
    // projection, and the same pixels under an identity matrix
    v3d::moya::RIBHandler distant;
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"perspective\" \"fov\" 45\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.4 -0.4 8  0.4 -0.4 8  0.4 0.4 8  -0.4 0.4 8]\n"
        "WorldEnd\n", &distant));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> distantPlanes = distant.context().picture();
    unsigned int distantCovered = 0;
    for (unsigned int row = 0; row < 48; row++) {
        for (unsigned int column = 0; column < 64; column++) {
            if (distantPlanes->value(v3d::moya::Planes::RED, column, row) > 0.5F) {
                distantCovered++;
            }
        }
    }
    BOOST_CHECK_GT(distantCovered, 0U);
    BOOST_CHECK_CLOSE(static_cast<float>(covered) / static_cast<float>(distantCovered), 4.0F, 15.0F);
}

/**
 * The screen window follows the frame aspect, so a frame that is not square does not stretch
 * the picture. A quad as wide as it is tall covers as many columns as rows.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_screen_window_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.5 -0.5 5  0.5 -0.5 5  0.5 0.5 5  -0.5 0.5 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();

    unsigned int columns = 0;
    for (unsigned int column = 0; column < 64; column++) {
        if (planes->value(v3d::moya::Planes::RED, column, 24) > 0.5F) {
            columns++;
        }
    }
    unsigned int rows = 0;
    for (unsigned int row = 0; row < 48; row++) {
        if (planes->value(v3d::moya::Planes::RED, 32, row) > 0.5F) {
            rows++;
        }
    }

    BOOST_CHECK_GT(columns, 0U);
    // one pixel of slack for where the edges land against the sample centres
    BOOST_CHECK_LE(columns > rows ? columns - rows : rows - columns, 1U);
}

/**
 * An explicit screen window replaces the one the frame aspect gives, and a later RiFormat
 * does not restore the default.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_explicit_screen_window_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "ScreenWindow -2 2 -2 2\n"
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-1.9 -1.9 5  1.9 -1.9 5  1.9 1.9 5  -1.9 1.9 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    // under a window of [-2, 2] the quad nearly fills the frame. The default [-4/3, 4/3] by
    // [-1, 1] would clip it at the top and bottom
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 32, 1), 1.0F);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 32, 46), 1.0F);
}

/**
 * A size given to the handler, as a command line gives one, replaces the size a scene's Format
 * names. The scene renders at the handler's size.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_resolution_override_test) {
    v3d::moya::RIBHandler handler;
    handler.resolution(8, 4);

    BOOST_REQUIRE(read("Format 64 48 1\nWorldBegin\nWorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().picture()->width(), 8U);
    BOOST_CHECK_EQUAL(handler.context().picture()->height(), 4U);
}

/**
 * A size given to the handler with a side of zero or less, or above largestResolution, is
 * refused. The scene's Format is used instead.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_resolution_refuses_a_side_out_of_range_test) {
    v3d::moya::RIBHandler handler;
    BOOST_CHECK(!handler.resolution(70000, 4));
    BOOST_CHECK(!handler.resolution(0, 4));
    BOOST_CHECK(!handler.resolution(8, -4));

    BOOST_REQUIRE(read("Format 64 48 1\nWorldBegin\nWorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().picture()->width(), 64U);
    BOOST_CHECK_EQUAL(handler.context().picture()->height(), 48U);
}

/**
 * A Format side of zero or less is moya's default for that side, 320 by 240, as RI reads it.
 * A side above largestResolution is skipped, and the size already set is kept.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_format_nonpositive_side_is_the_default_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read("Format 64 0 1\nFormat 70000 16 1\nWorldBegin\nWorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().picture()->width(), 64U);
    BOOST_CHECK_EQUAL(handler.context().picture()->height(), 240U);
}

/**
 * RI freezes the camera options at WorldBegin, so a screen window named after the projection
 * still frames the picture. A window from 0 to 2 puts a quad spanning -1 to 1 in the left half.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_screen_window_after_projection_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 64 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "ScreenWindow 0 2 -1 1\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Surface \"constant\"\n"
        "Polygon \"P\" [-1 -1 5  1 -1 5  1 1 5  -1 1 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    BOOST_CHECK_GT(planes->value(v3d::moya::Planes::RED, 16, 32), 0.5F);
    BOOST_CHECK_LT(planes->value(v3d::moya::Planes::RED, 48, 32), 0.5F);
}

/**
 * The RenderMan Interface specification's unit cube pins RiRotate's sign. Its RIB entity file
 * example draws one polygon at z = +0.5, the far face in the left-handed camera space, under
 * six rotations, and names the face each one gives. The traced scene holds every polygon in
 * world space as two triangles, the second sharing the first's diagonal, so the midpoint of the
 * first triangle's a and c is the face's centre.
 *
 * Covers the base: e0859388 gave RenderContext::rotate its present sign. Checked by negating the
 * angle there, which fails this case on the right, left, bottom and top faces.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_rotations_build_the_specification_cube_test) {
    v3d::moya::RIBHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(reader.read("data/spec-cube.rib", &handler));

    const std::vector<const v3d::render::offline::trace::Triangle*> triangles =
        handler.context().traced().all<v3d::render::offline::trace::Triangle>();
    BOOST_REQUIRE_EQUAL(triangles.size(), 12U);

    // the faces in the order the file draws them, as the specification names them
    const char* const names[] = { "far", "right", "near", "left", "bottom", "top" };
    const glm::vec3 centres[] = {
        { 0.0F, 0.0F, 0.5F }, { 0.5F, 0.0F, 0.0F }, { 0.0F, 0.0F, -0.5F },
        { -0.5F, 0.0F, 0.0F }, { 0.0F, -0.5F, 0.0F }, { 0.0F, 0.5F, 0.0F }
    };
    // the camera sits five units back, so the cube's centre is five units along +z
    const glm::mat4x4 toCamera = handler.context().traced().camera().view;
    const glm::vec3 cube(0.0F, 0.0F, 5.0F);
    for (std::size_t face = 0; face < 6; face++) {
        BOOST_TEST_CONTEXT("the " << names[face] << " face") {
            const v3d::render::offline::trace::Triangle & first = *triangles[face * 2];
            const glm::vec3 world = 0.5F * (first.a() + first.c());
            const glm::vec3 centre = glm::vec3(toCamera * glm::vec4(world, 1.0F)) - cube;
            BOOST_TEST(glm::length(centre - centres[face]) < 1.0e-5F);
        }
    }
}

/**
 * An unknown projection is logged by name.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_an_unknown_projection_is_logged_test) {
    v3d::moya::RIBHandler handler;
    const Heard heard(handler.context().logger()->get());

    BOOST_REQUIRE(read("Projection \"fisheye\"\n", &handler));

    BOOST_TEST(heard.said("fisheye"));
}

};  // namespace
};  // namespace v3d::moya::handling
