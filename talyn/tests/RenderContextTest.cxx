/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <talyn/libtalyn/RenderContext.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * A 16 by 16 frame of an orthographic camera looking down +z from z = -1, with a square
 * pixel so that the frame covers world x and y over [-1, 1]. A pixel centre is then at
 * x = -1 + (column + 0.5) / 8 and y = 1 - (row + 0.5) / 8.
 **/
const unsigned int SIZE = 16;

void frame(v3d::talyn::RenderContext & rc) {
    rc.format(SIZE, SIZE);
    v3d::type::camera::Profile & profile = rc.scene().camera().profile();
    profile.orthographic(true);
    profile.pixelAspect(1.0f);
    profile.orthoZoom(1.0f);
    profile.eye(glm::vec3(0.0f, 0.0f, -1.0f));
    profile.clipping(0.001f, 100.0f);
}

void checkPixel(const v3d::render::offline::FrameBuffer & buffer,
    unsigned int column, unsigned int row, const glm::vec3 & expected) {
    BOOST_TEST_CONTEXT("pixel " << column << ", " << row) {
        BOOST_CHECK_EQUAL(buffer.value(0, column, row), expected.r);
        BOOST_CHECK_EQUAL(buffer.value(1, column, row), expected.g);
        BOOST_CHECK_EQUAL(buffer.value(2, column, row), expected.b);
    }
}

};  // namespace

BOOST_AUTO_TEST_CASE(rendercontext_background_test) {
    v3d::talyn::RenderContext rc;
    frame(rc);
    const glm::vec3 background(0.0f, 0.0f, 1.0f);
    rc.scene().background(background);

    rc.render();

    auto buffer = rc.framebuffer();
    BOOST_REQUIRE(buffer);
    // every ray misses, so the whole frame is the background
    checkPixel(*buffer, 0, 0, background);
    checkPixel(*buffer, 8, 8, background);
    // and it is opaque, which an unwritten alpha plane would not be
    BOOST_CHECK_EQUAL(buffer->value(3, 8, 8), 1.0f);
}

BOOST_AUTO_TEST_CASE(rendercontext_triangle_test) {
    v3d::talyn::RenderContext rc;
    frame(rc);

    const glm::vec3 background(0.0f, 0.0f, 1.0f);
    const glm::vec3 red(1.0f, 0.0f, 0.0f);
    rc.scene().background(background);

    // a right triangle with its vertical edge at x = 0.5 and its horizontal one at y = -0.5,
    // so a pixel either side of each edge is half a pixel away from it
    rc.scene().add(v3d::talyn::Triangle(
        glm::vec3(-1.5f, -0.5f, 1.0f),
        glm::vec3(0.5f, -0.5f, 1.0f),
        glm::vec3(0.5f, 1.5f, 1.0f),
        red));

    rc.render();

    auto buffer = rc.framebuffer();
    BOOST_REQUIRE(buffer);

    // inside, near the middle of the frame
    checkPixel(*buffer, 8, 8, red);
    // outside, past the hypotenuse in the upper left corner
    checkPixel(*buffer, 0, 0, background);
    // either side of the vertical edge at x = 0.5, which falls between columns 11 and 12
    checkPixel(*buffer, 11, 8, red);
    checkPixel(*buffer, 12, 8, background);
    // either side of the horizontal edge at y = -0.5, which falls between rows 11 and 12.
    // Image row 0 is the top of the picture and the camera measures y downward, so the
    // larger row index is the lower half of the frame
    checkPixel(*buffer, 8, 11, red);
    checkPixel(*buffer, 8, 12, background);
}

BOOST_AUTO_TEST_CASE(rendercontext_nearest_hit_test) {
    v3d::talyn::RenderContext rc;
    frame(rc);

    const glm::vec3 closer(1.0f, 0.0f, 0.0f);
    const glm::vec3 further(0.0f, 1.0f, 0.0f);

    // two triangles over the same pixel, the first one further from the camera
    rc.scene().add(v3d::talyn::Triangle(
        glm::vec3(-1.0f, -1.0f, 5.0f), glm::vec3(1.0f, -1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 5.0f), further));
    rc.scene().add(v3d::talyn::Triangle(
        glm::vec3(-1.0f, -1.0f, 2.0f), glm::vec3(1.0f, -1.0f, 2.0f), glm::vec3(0.0f, 1.0f, 2.0f), closer));

    rc.render();

    // the nearer one wins whichever order they were added in
    checkPixel(*rc.framebuffer(), 8, 8, closer);
}

BOOST_AUTO_TEST_CASE(rendercontext_unformatted_test) {
    v3d::talyn::RenderContext rc;

    // a scene that named no format has nothing to draw into, and rendering it is not a crash
    rc.render();

    BOOST_CHECK(!rc.framebuffer());
}
