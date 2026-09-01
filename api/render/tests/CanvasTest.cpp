/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstddef>

#include <boost/test/unit_test.hpp>

#include "../realtime/Canvas.h"

#include "../../font/TextBuffer.h"

#include <glm/vec4.hpp>

namespace {

    const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);

    /**
     * The point one corner of the canvas maps to, for checking the projection without
     * pulling glm's matrix multiply into the expectation.
     **/
    glm::vec2 project(const v3d::render::realtime::Canvas& canvas, const glm::vec2& point) {
        const glm::mat4 projection = canvas.projection();
        return glm::vec2(
            projection[0][0] * point.x + projection[3][0],
            projection[1][1] * point.y + projection[3][1]);
    }

};  // namespace

BOOST_AUTO_TEST_SUITE(canvas_test)

/**
 * A canvas nothing has been drawn on produces no draw at all, so a frame with no 2D content
 * costs neither an upload nor a batch.
 **/
BOOST_AUTO_TEST_CASE(an_empty_canvas_has_nothing_to_draw) {
    v3d::render::realtime::Canvas canvas;

    BOOST_CHECK(canvas.empty());
    BOOST_CHECK_EQUAL(canvas.batches().size(), 0);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 0);
}

/**
 * A rectangle is four vertices and two triangles.
 **/
BOOST_AUTO_TEST_CASE(a_rectangle_is_two_triangles) {
    v3d::render::realtime::Canvas canvas;
    canvas.rect(glm::vec2(10.0f, 20.0f), glm::vec2(30.0f, 50.0f), white);

    BOOST_CHECK(!canvas.empty());
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6);
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.batches()[0].indices, 6);
    BOOST_CHECK_EQUAL(canvas.batches()[0].firstIndex, 0);
    // an untextured quad names no texture - the renderer draws it against its white one
    BOOST_CHECK(!canvas.batches()[0].texture.valid());
}

/**
 * The batch is cut where the texture changes and nowhere else, so a screen of quads sharing
 * an atlas is one draw.
 **/
BOOST_AUTO_TEST_CASE(batches_are_cut_only_where_the_texture_changes) {
    v3d::render::realtime::Canvas canvas;
    const v3d::render::realtime::TextureHandle first(3);
    const v3d::render::realtime::TextureHandle second(7);

    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white);
    canvas.rect(glm::vec2(2.0f, 0.0f), glm::vec2(3.0f, 1.0f), red);
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, first);
    canvas.rect(glm::vec2(4.0f, 0.0f), glm::vec2(5.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, first);
    canvas.rect(glm::vec2(6.0f, 0.0f), glm::vec2(7.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, second);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 3);
    // the two untextured quads share a batch even though their colours differ
    BOOST_CHECK_EQUAL(canvas.batches()[0].indices, 12);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 12);
    BOOST_CHECK_EQUAL(canvas.batches()[1].firstIndex, 12);
    BOOST_CHECK_EQUAL(canvas.batches()[2].indices, 6);
    BOOST_CHECK_EQUAL(canvas.batches()[2].firstIndex, 24);
}

/**
 * Going back to a texture that was used earlier starts a new batch rather than reopening the
 * old one - the quads in between have to stay underneath, and merging them would put them on
 * top. Painter order beats batch count.
 **/
BOOST_AUTO_TEST_CASE(returning_to_a_texture_does_not_reopen_its_batch) {
    v3d::render::realtime::Canvas canvas;
    const v3d::render::realtime::TextureHandle atlas(1);

    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, atlas);
    canvas.rect(glm::vec2(2.0f, 0.0f), glm::vec2(3.0f, 1.0f), white);
    canvas.rect(glm::vec2(4.0f, 0.0f), glm::vec2(5.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white, atlas);

    BOOST_CHECK_EQUAL(canvas.batches().size(), 3);
}

/**
 * The transform applies as vertices are added, so a translate moves what is drawn after it
 * and a pop puts it back.
 **/
BOOST_AUTO_TEST_CASE(the_transform_stack_offsets_what_is_drawn_under_it) {
    v3d::render::realtime::Canvas canvas;

    canvas.push();
    canvas.translate(glm::vec2(100.0f, 200.0f));
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 10.0f), white);
    canvas.pop();
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 10.0f), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 100.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 200.0f, 0.001f);
    BOOST_CHECK_SMALL(canvas.vertices()[4].position.x, 0.001f);
    BOOST_CHECK_SMALL(canvas.vertices()[4].position.y, 0.001f);
}

/**
 * A pop with nothing pushed leaves the canvas's own identity in place rather than emptying
 * the stack - an unbalanced pop in one caller must not corrupt every draw after it.
 **/
BOOST_AUTO_TEST_CASE(an_unbalanced_pop_leaves_the_identity_transform) {
    v3d::render::realtime::Canvas canvas;

    canvas.pop();
    canvas.pop();
    canvas.rect(glm::vec2(5.0f, 6.0f), glm::vec2(7.0f, 8.0f), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 5.0f, 0.001f);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 6.0f, 0.001f);
}

/**
 * clear() drops the geometry and the transform stack but keeps the size, since the canvas is
 * refilled every frame and resized only when the window is.
 **/
BOOST_AUTO_TEST_CASE(clearing_keeps_the_size_and_resets_the_transform) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    canvas.push();
    canvas.translate(glm::vec2(50.0f, 50.0f));
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white);
    canvas.clear();
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white);

    BOOST_CHECK_EQUAL(canvas.width(), 800);
    BOOST_CHECK_EQUAL(canvas.height(), 600);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_SMALL(canvas.vertices()[0].position.x, 0.001f);
    BOOST_CHECK_SMALL(canvas.vertices()[0].position.y, 0.001f);
}

/**
 * The projection puts pixel (0,0) at the top left of clip space and the far corner at the
 * bottom right. Getting the y direction wrong draws the whole frame upside down.
 **/
BOOST_AUTO_TEST_CASE(the_projection_puts_the_origin_at_the_top_left) {
    v3d::render::realtime::Canvas canvas;
    canvas.resize(800, 600);

    const glm::vec2 topLeft = project(canvas, glm::vec2(0.0f, 0.0f));
    const glm::vec2 bottomRight = project(canvas, glm::vec2(800.0f, 600.0f));
    const glm::vec2 centre = project(canvas, glm::vec2(400.0f, 300.0f));

    BOOST_CHECK_CLOSE(topLeft.x, -1.0f, 0.001f);
    BOOST_CHECK_CLOSE(topLeft.y, -1.0f, 0.001f);
    BOOST_CHECK_CLOSE(bottomRight.x, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(bottomRight.y, 1.0f, 0.001f);
    BOOST_CHECK_SMALL(centre.x, 0.001f);
    BOOST_CHECK_SMALL(centre.y, 0.001f);
}

/**
 * A canvas that has never been sized still produces a usable projection rather than dividing
 * by zero - a window can report no area before the first resize.
 **/
BOOST_AUTO_TEST_CASE(an_unsized_canvas_has_a_finite_projection) {
    v3d::render::realtime::Canvas canvas;

    const glm::mat4 projection = canvas.projection();

    BOOST_CHECK(projection[0][0] > 0.0f);
    BOOST_CHECK(projection[1][1] > 0.0f);
}

/**
 * A circle is a fan of triangles, and it batches with the untextured quads around it.
 **/
BOOST_AUTO_TEST_CASE(a_circle_is_a_fan_that_batches_with_untextured_quads) {
    v3d::render::realtime::Canvas canvas;

    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white);
    canvas.circle(glm::vec2(50.0f, 50.0f), 10.0f, 8, white);

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6 + 8 * 3);
    // the centre, one vertex per side, and a repeat of the first so the wrap needs no case
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4 + 1 + 9);
}

/**
 * Too few sides to be a shape draws nothing rather than emitting degenerate triangles.
 **/
BOOST_AUTO_TEST_CASE(a_circle_of_fewer_than_three_sides_draws_nothing) {
    v3d::render::realtime::Canvas canvas;

    canvas.circle(glm::vec2(0.0f, 0.0f), 10.0f, 2, white);

    BOOST_CHECK(canvas.empty());
}

/**
 * Text is copied out of a laid out text buffer against the atlas it was packed into, with
 * the buffer's indices rebased onto the canvas's own vertex stream.
 **/
BOOST_AUTO_TEST_CASE(text_is_appended_against_its_atlas_and_reindexed) {
    v3d::render::realtime::Canvas canvas;
    const v3d::render::realtime::TextureHandle atlas(4);

    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white);

    // one glyph quad, as a text buffer would have laid it out
    v3d::font::TextBuffer buffer;
    for (int corner = 0; corner < 4; corner++) {
        buffer.addVertex(glm::vec3(static_cast<float>(corner), 0.0f, 0.0f));
        buffer.addTextureCoordinate(glm::vec2(0.0f, 0.0f));
        buffer.addColor(white);
    }
    const unsigned int order[6] = {0, 2, 1, 0, 3, 2};
    for (int index = 0; index < 6; index++) {
        buffer.addIndex(order[index]);
    }

    canvas.text(buffer, atlas);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 2);
    BOOST_CHECK(canvas.batches()[1].texture == atlas);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 6);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 8);
    // the glyph's index 0 has to become index 4, since the rectangle is already in the stream
    BOOST_CHECK_EQUAL(canvas.indices()[6], 4);
}

/**
 * A text buffer with nothing in it adds neither vertices nor a batch, so drawing an empty
 * string does not cut the batch a caller is in the middle of.
 **/
BOOST_AUTO_TEST_CASE(empty_text_adds_no_batch) {
    v3d::render::realtime::Canvas canvas;

    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), white);
    v3d::font::TextBuffer buffer;
    canvas.text(buffer, v3d::render::realtime::TextureHandle(9));

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
