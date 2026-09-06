/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cmath>
#include <cstdlib>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../realtime/LineCanvas.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>

namespace {

constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
constexpr glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);

/**
 * A quarter turn about z, which takes the x axis onto the y axis.
 **/
glm::mat4 quarterTurn() {
    return glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

};  // namespace

BOOST_AUTO_TEST_SUITE(linecanvas_test)

/**
 * A canvas nothing has been drawn on produces no draw at all, so a frame with no lines in it
 * costs neither an upload nor an item.
 **/
BOOST_AUTO_TEST_CASE(an_empty_canvas_has_nothing_to_draw) {
    v3d::render::realtime::LineCanvas canvas;

    BOOST_CHECK(canvas.empty());
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 0);
}

/**
 * A line list carries two vertices per segment and no indices.
 **/
BOOST_AUTO_TEST_CASE(a_segment_is_two_vertices) {
    v3d::render::realtime::LineCanvas canvas;
    canvas.line(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 2.0f, 3.0f), red);

    BOOST_CHECK(!canvas.empty());
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 2);
    BOOST_CHECK_EQUAL(canvas.vertices()[0].position.x, 0.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[1].position.x, 1.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[1].position.y, 2.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[1].position.z, 3.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[1].colour.r, 1.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[1].colour.g, 0.0f);
}

/**
 * Clearing drops the stream and the transform stack with it, since the next tick rebuilds
 * both from nothing.
 **/
BOOST_AUTO_TEST_CASE(clearing_drops_the_stream) {
    v3d::render::realtime::LineCanvas canvas;
    canvas.push();
    canvas.translate(glm::vec3(5.0f, 0.0f, 0.0f));
    canvas.line(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), white);

    canvas.clear();

    BOOST_CHECK(canvas.empty());
    canvas.line(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), white);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 2);
    BOOST_CHECK_EQUAL(canvas.vertices()[0].position.x, 0.0f);
}

/**
 * A polyline is its segments, and a closed one is a segment more. Fewer than two points is
 * not a line and draws nothing.
 **/
BOOST_AUTO_TEST_CASE(a_polyline_joins_its_points_in_order) {
    v3d::render::realtime::LineCanvas canvas;

    std::vector<glm::vec3> lone;
    lone.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    canvas.polyline(lone, white);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 0);

    std::vector<glm::vec3> points;
    points.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    points.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    points.push_back(glm::vec3(1.0f, 1.0f, 0.0f));

    canvas.polyline(points, white);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4);

    canvas.clear();
    canvas.polyline(points, white, true);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 6);
    // the closing segment runs from the last point back to the first
    BOOST_CHECK_EQUAL(canvas.vertices()[4].position.y, 1.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[5].position.x, 0.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[5].position.y, 0.0f);
}

/**
 * A box is twelve edges - two faces and the four struts between them - and every vertex of
 * it is a corner.
 **/
BOOST_AUTO_TEST_CASE(a_box_is_twelve_edges_between_its_corners) {
    v3d::render::realtime::LineCanvas canvas;
    canvas.box(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 24);
    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK_EQUAL(std::abs(vertex.position.x), 1.0f);
        BOOST_CHECK_EQUAL(std::abs(vertex.position.y), 1.0f);
        BOOST_CHECK_EQUAL(std::abs(vertex.position.z), 1.0f);
    }
}

/**
 * A ring lies in the plane its two axes span, every point of it is on the rim, and the last
 * segment lands back on the first point.
 **/
BOOST_AUTO_TEST_CASE(a_circle_closes_in_the_plane_of_its_axes) {
    v3d::render::realtime::LineCanvas canvas;
    const glm::vec3 u(1.0f, 0.0f, 0.0f);
    const glm::vec3 v(0.0f, 0.0f, 1.0f);

    canvas.circle(glm::vec3(0.0f), u, v, 2.0f, 8, white);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 16);

    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK_SMALL(vertex.position.y, 0.001f);
        const float radius = std::sqrt(vertex.position.x * vertex.position.x + vertex.position.z * vertex.position.z);
        BOOST_CHECK_CLOSE(radius, 2.0f, 0.01f);
    }

    BOOST_CHECK_CLOSE(canvas.vertices().front().position.x, canvas.vertices().back().position.x, 0.01f);
    BOOST_CHECK_SMALL(canvas.vertices().back().position.z, 0.001f);
}

/**
 * Fewer than three sides is not a ring.
 **/
BOOST_AUTO_TEST_CASE(a_circle_of_fewer_than_three_sides_draws_nothing) {
    v3d::render::realtime::LineCanvas canvas;
    canvas.circle(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 2, white);

    BOOST_CHECK(canvas.empty());
}

/**
 * The transform applies on the cpu as vertices are added, so geometry can be described at
 * the origin and placed where it is drawn.
 **/
BOOST_AUTO_TEST_CASE(the_transform_stack_places_what_is_drawn_under_it) {
    v3d::render::realtime::LineCanvas canvas;

    canvas.translate(glm::vec3(10.0f, 0.0f, 0.0f));
    canvas.line(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), white);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 2);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(canvas.vertices()[1].position.x, 11.0f, 0.01f);

    // a pushed transform composes onto the one under it: the segment runs up the y axis now,
    // and still starts from the outer translation
    canvas.push();
    canvas.transform(quarterTurn());
    canvas.line(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), white);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.x, 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(canvas.vertices()[3].position.x, 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(canvas.vertices()[3].position.y, 1.0f, 0.01f);

    // and a pop puts back what was there before it
    canvas.pop();
    canvas.line(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), white);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 6);
    BOOST_CHECK_CLOSE(canvas.vertices()[5].position.x, 11.0f, 0.01f);
}

/**
 * A translation is in the frame it is applied to rather than in the world, so an offset
 * under a rotation moves along the rotated axis.
 **/
BOOST_AUTO_TEST_CASE(a_translation_follows_the_transform_it_is_applied_under) {
    v3d::render::realtime::LineCanvas canvas;

    canvas.transform(quarterTurn());
    canvas.translate(glm::vec3(2.0f, 0.0f, 0.0f));
    canvas.line(glm::vec3(0.0f), glm::vec3(0.0f), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 2);
    BOOST_CHECK_SMALL(canvas.vertices()[0].position.x, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 2.0f, 0.01f);
}

/**
 * The identity at the bottom of the stack is the canvas's own, so an unbalanced pop leaves
 * the canvas usable rather than reading off the end of the stack.
 **/
BOOST_AUTO_TEST_CASE(popping_past_the_bottom_of_the_stack_is_harmless) {
    v3d::render::realtime::LineCanvas canvas;

    canvas.pop();
    canvas.pop();
    canvas.line(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 2);
    BOOST_CHECK_EQUAL(canvas.vertices()[0].position.x, 0.0f);
    BOOST_CHECK_EQUAL(canvas.vertices()[1].position.x, 1.0f);
}

BOOST_AUTO_TEST_SUITE_END()
