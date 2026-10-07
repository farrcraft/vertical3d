/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DepthOrder.h>
#include <api/render/realtime/WorldCanvas.h>

#include <cstddef>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using v3d::render::realtime::DepthOrder;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::WorldCanvas;

namespace {

constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

/**
 * A unit square on the ground plane, so the first vertex a quad writes says which quad it was.
 **/
WorldCanvas::Corners tile(float x, float z) {
    return {
        glm::vec3(x, 0.0f, z),
        glm::vec3(x + 1.0f, 0.0f, z),
        glm::vec3(x + 1.0f, 0.0f, z + 1.0f),
        glm::vec3(x, 0.0f, z + 1.0f)
    };
}

/**
 * @return the x of the first corner of the nth quad the canvas holds
 **/
float quadAt(const WorldCanvas& canvas, std::size_t n) {
    return canvas.vertices()[n * 4].position.x;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(depth_order_test)

/**
 * Quads added nearest first come out furthest first, which is the order a painter draws in.
 **/
BOOST_AUTO_TEST_CASE(quads_come_out_furthest_first) {
    DepthOrder order;
    order.quad(1.0f, tile(0.0f, 0.0f), white);
    order.quad(2.0f, tile(1.0f, 0.0f), white);
    order.quad(3.0f, tile(2.0f, 0.0f), white);

    WorldCanvas canvas;
    order.into(&canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 12u);
    BOOST_CHECK_EQUAL(quadAt(canvas, 0), 2.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 1), 1.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 2), 0.0f);
}

/**
 * Equal keys with alternating textures are grouped by texture, so the canvas cuts two batches
 * where submission order would have cut four.
 **/
BOOST_AUTO_TEST_CASE(equal_keys_are_grouped_by_texture) {
    const TextureHandle a(1);
    const TextureHandle b(2);
    DepthOrder order;
    order.quad(5.0f, tile(0.0f, 0.0f), glm::vec2(0.0f), glm::vec2(1.0f), white, a);
    order.quad(5.0f, tile(1.0f, 0.0f), glm::vec2(0.0f), glm::vec2(1.0f), white, b);
    order.quad(5.0f, tile(2.0f, 0.0f), glm::vec2(0.0f), glm::vec2(1.0f), white, a);
    order.quad(5.0f, tile(3.0f, 0.0f), glm::vec2(0.0f), glm::vec2(1.0f), white, b);

    WorldCanvas canvas;
    order.into(&canvas);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 2u);
    BOOST_CHECK(canvas.batches()[0].texture == a);
    BOOST_CHECK(canvas.batches()[1].texture == b);
}

/**
 * Equal keys and one texture keep the order they were added in, so a caller that gets its
 * keys equal on purpose still decides what is on top.
 **/
BOOST_AUTO_TEST_CASE(exact_equals_keep_their_submission_order) {
    DepthOrder order;
    order.quad(1.0f, tile(0.0f, 0.0f), white);
    order.quad(1.0f, tile(1.0f, 0.0f), white);
    order.quad(1.0f, tile(2.0f, 0.0f), white);

    WorldCanvas canvas;
    order.into(&canvas);

    BOOST_CHECK_EQUAL(quadAt(canvas, 0), 0.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 1), 1.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 2), 2.0f);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1u);
}

/**
 * What the helper hands over goes through the canvas's transform like anything else added
 * to it.
 **/
BOOST_AUTO_TEST_CASE(the_canvas_transform_applies) {
    DepthOrder order;
    order.quad(1.0f, tile(0.0f, 0.0f), white);

    WorldCanvas canvas;
    canvas.translate(glm::vec3(10.0f, 0.0f, 0.0f));
    order.into(&canvas);

    BOOST_CHECK_EQUAL(quadAt(canvas, 0), 10.0f);
}

/**
 * A depth order handed to a tinted canvas is tinted, since it adds through the canvas. One
 * tint therefore covers every sprite and particle a frame sorts.
 **/
BOOST_AUTO_TEST_CASE(an_order_handed_to_a_tinted_canvas_is_tinted) {
    DepthOrder order;
    order.quad(1.0f, tile(0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));

    WorldCanvas canvas;
    canvas.tint(glm::vec4(0.25f, 0.5f, 1.0f, 1.0f));
    order.into(&canvas);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4u);
    BOOST_TEST((canvas.vertices()[0].colour == glm::vec4(0.25f, 0.5f, 1.0f, 0.5f)));
}

/**
 * A key that is not a number is drawn after every key that is, and the rest keep their order.
 **/
BOOST_AUTO_TEST_CASE(a_key_that_is_not_a_number_is_drawn_last) {
    DepthOrder order;
    order.quad(std::nanf(""), tile(0.0f, 0.0f), white);
    order.quad(1.0f, tile(1.0f, 0.0f), white);
    order.quad(std::nanf(""), tile(2.0f, 0.0f), white);
    order.quad(2.0f, tile(3.0f, 0.0f), white);

    WorldCanvas canvas;
    order.into(&canvas);

    BOOST_CHECK_EQUAL(quadAt(canvas, 0), 3.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 1), 1.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 2), 0.0f);
    BOOST_CHECK_EQUAL(quadAt(canvas, 3), 2.0f);
}

BOOST_AUTO_TEST_SUITE_END()
