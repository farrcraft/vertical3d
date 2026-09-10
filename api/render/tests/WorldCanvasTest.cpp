/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/WorldCanvas.h>

#include <array>
#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>

namespace {

constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
constexpr glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);

/**
 * A unit square lying on the ground plane, in the perimeter order grid::tileCorners uses.
 **/
v3d::render::realtime::WorldCanvas::Corners tile(float x, float z) {
    return {
        glm::vec3(x, 0.0f, z),
        glm::vec3(x + 1.0f, 0.0f, z),
        glm::vec3(x + 1.0f, 0.0f, z + 1.0f),
        glm::vec3(x, 0.0f, z + 1.0f)
    };
}

v3d::render::realtime::TextureHandle handle(uint32_t id) {
    return v3d::render::realtime::TextureHandle(id);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(worldcanvas_test)

/**
 * A canvas nothing has been drawn on produces no draw at all, so a frame with no world quads
 * in it costs neither an upload nor an item.
 **/
BOOST_AUTO_TEST_CASE(an_empty_canvas_has_nothing_to_draw) {
    v3d::render::realtime::WorldCanvas canvas;

    BOOST_CHECK(canvas.empty());
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 0);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 0);
}

/**
 * A quad is four vertices and six indices, fanned from the first corner so that the corners
 * come out in the order they were given.
 **/
BOOST_AUTO_TEST_CASE(a_quad_is_four_world_vertices_and_two_triangles) {
    v3d::render::realtime::WorldCanvas canvas;
    canvas.quad(tile(0.0f, 0.0f), white);

    BOOST_CHECK(!canvas.empty());
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_REQUIRE_EQUAL(canvas.indices().size(), 6);

    // the position is a world position and keeps all three of its components
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.x, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.y, 0.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.z, 1.0f, 0.001f);

    const std::vector<uint32_t> expected = {0, 1, 2, 2, 3, 0};
    for (std::size_t index = 0; index < expected.size(); index++) {
        BOOST_CHECK_EQUAL(canvas.indices()[index], expected[index]);
    }

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.batches()[0].indices, 6);
    // an untextured quad names no texture and is drawn against the renderer's white one
    BOOST_CHECK(!canvas.batches()[0].texture.valid());
}

/**
 * The uv rectangle is laid over the corners in the order they are given, so a sprite packed
 * into an atlas comes out the right way up.
 **/
BOOST_AUTO_TEST_CASE(the_uv_rectangle_follows_the_corner_order) {
    v3d::render::realtime::WorldCanvas canvas;
    canvas.quad(tile(0.0f, 0.0f), glm::vec2(0.25f, 0.5f), glm::vec2(0.75f, 1.0f), white, handle(3));

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].uv.x, 0.25f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].uv.y, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[1].uv.x, 0.75f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[1].uv.y, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].uv.x, 0.75f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].uv.y, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[3].uv.x, 0.25f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[3].uv.y, 1.0f, 0.001f);
}

/**
 * The stream cuts where the bound texture changes and nowhere else, which is ADR-0005's rule
 * unchanged - so a sheet of sprites drawn from one atlas is one draw however many there are.
 **/
BOOST_AUTO_TEST_CASE(the_stream_cuts_where_the_texture_changes) {
    v3d::render::realtime::WorldCanvas canvas;
    canvas.quad(tile(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, handle(1));
    canvas.quad(tile(1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, handle(1));
    canvas.quad(tile(2.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, handle(1));

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.batches()[0].indices, 18);

    // a second texture is a second draw, and going back to the first is a third
    canvas.quad(tile(3.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, handle(2));
    canvas.quad(tile(4.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, handle(1));
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 3);
    BOOST_CHECK_EQUAL(canvas.batches()[1].firstIndex, 18);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 6);
    BOOST_CHECK_EQUAL(canvas.batches()[2].firstIndex, 24);

    // an untextured quad cuts the batch too, because white is a texture like any other
    canvas.quad(tile(5.0f, 0.0f), red);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 4);
}

/**
 * The order is the order quads were added, per ADR-0042: the caller decides what is in front
 * of what, because in an isometric projection that is a fact about the game and not about the
 * distance from the camera.
 **/
BOOST_AUTO_TEST_CASE(quads_are_drawn_in_the_order_they_were_added) {
    v3d::render::realtime::WorldCanvas canvas;
    canvas.quad(tile(0.0f, 8.0f), white);   // far up the ground plane, added first
    canvas.quad(tile(0.0f, 0.0f), red);     // near the camera, added second

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.z, 8.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[4].position.z, 0.0f, 0.001f);
    // one batch, because both are untextured - painter order inside it is index order
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.indices()[6], 4u);
}

/**
 * The transform stack applies on the cpu as vertices are added, so geometry can be described
 * at the origin and placed where it is drawn - and it is a 4x4 over a world position, unlike
 * Canvas's, which only ever moves a point in a plane.
 **/
BOOST_AUTO_TEST_CASE(the_transform_stack_places_geometry) {
    v3d::render::realtime::WorldCanvas canvas;

    canvas.push();
    canvas.translate(glm::vec3(10.0f, 2.0f, -3.0f));
    canvas.quad(tile(0.0f, 0.0f), white);
    canvas.pop();

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 10.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 2.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.z, -3.0f, 0.001f);

    // and popping put it back, so the next quad is where it says it is
    canvas.quad(tile(0.0f, 0.0f), white);
    BOOST_CHECK_CLOSE(canvas.vertices()[4].position.x, 0.0f, 0.001f);

    // a rotation is expressible, which is what makes a quad standing upright possible
    canvas.push();
    canvas.transform(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    canvas.quad(tile(0.0f, 0.0f), white);
    canvas.pop();
    // the ground plane turned up to face the camera: z became -y
    BOOST_CHECK_CLOSE(canvas.vertices()[10].position.y, -1.0f, 0.001f);
    BOOST_CHECK_SMALL(canvas.vertices()[10].position.z, 0.001f);
}

/**
 * Clearing drops the stream and the stack, so a canvas rebuilt every frame does not carry a
 * transform a caller forgot to pop.
 **/
BOOST_AUTO_TEST_CASE(clearing_drops_the_stream_and_the_stack) {
    v3d::render::realtime::WorldCanvas canvas;
    canvas.push();
    canvas.translate(glm::vec3(5.0f, 5.0f, 5.0f));
    canvas.quad(tile(0.0f, 0.0f), white);

    canvas.clear();
    BOOST_CHECK(canvas.empty());
    BOOST_CHECK_EQUAL(canvas.batches().size(), 0);

    canvas.quad(tile(0.0f, 0.0f), white);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 0.0f, 0.001f);
}

BOOST_AUTO_TEST_SUITE_END()
