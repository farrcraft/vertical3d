/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

constexpr glm::vec4 white(1.0F, 1.0F, 1.0F, 1.0F);
constexpr glm::vec4 red(1.0F, 0.0F, 0.0F, 1.0F);

/**
 * Where a point of the canvas lands in clip space. It applies the projection's scale and offset
 * directly, so the expectation does not depend on glm's matrix multiply.
 **/
glm::vec2 project(const v3d::render::realtime::canvas::Canvas& canvas, const glm::vec2& point) {
    const glm::mat4 projection = canvas.projection();
    return glm::vec2(
        (projection[0][0] * point.x) + projection[3][0],
        (projection[1][1] * point.y) + projection[3][1]);
}

BOOST_AUTO_TEST_SUITE(canvas_test)

/**
 * A canvas nothing has been drawn on produces no draw at all, so a frame with no 2D content
 * costs neither an upload nor a batch.
 **/
BOOST_AUTO_TEST_CASE(an_empty_canvas_has_nothing_to_draw) {
    const v3d::render::realtime::canvas::Canvas canvas;

    BOOST_CHECK(canvas.empty());
    BOOST_CHECK_EQUAL(canvas.batches().size(), 0);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 0);
}

/**
 * A rectangle is four vertices and two triangles.
 **/
BOOST_AUTO_TEST_CASE(a_rectangle_is_two_triangles) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.rect(glm::vec2(10.0F, 20.0F), glm::vec2(30.0F, 50.0F), white);

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
 * A new batch starts where the texture changes and nowhere else, so a screen of quads sharing
 * an atlas is one draw.
 **/
BOOST_AUTO_TEST_CASE(batches_are_cut_only_where_the_texture_changes) {
    v3d::render::realtime::canvas::Canvas canvas;
    const v3d::render::realtime::TextureHandle first(3);
    const v3d::render::realtime::TextureHandle second(7);

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.rect(glm::vec2(2.0F, 0.0F), glm::vec2(3.0F, 1.0F), red);
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white, first);
    canvas.rect(glm::vec2(4.0F, 0.0F), glm::vec2(5.0F, 1.0F), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white, first);
    canvas.rect(glm::vec2(6.0F, 0.0F), glm::vec2(7.0F, 1.0F), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white, second);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 3);
    // the two untextured quads share a batch even though their colours differ
    BOOST_CHECK_EQUAL(canvas.batches()[0].indices, 12);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 12);
    BOOST_CHECK_EQUAL(canvas.batches()[1].firstIndex, 12);
    BOOST_CHECK_EQUAL(canvas.batches()[2].indices, 6);
    BOOST_CHECK_EQUAL(canvas.batches()[2].firstIndex, 24);
}

/**
 * Going back to a texture used earlier starts a new batch instead of adding to the old one. The
 * quads in between stay underneath, and merging the batches would change what is on top.
 * Painter's order takes priority over batch count.
 **/
BOOST_AUTO_TEST_CASE(returning_to_a_texture_does_not_reopen_its_batch) {
    v3d::render::realtime::canvas::Canvas canvas;
    const v3d::render::realtime::TextureHandle atlas(1);

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white, atlas);
    canvas.rect(glm::vec2(2.0F, 0.0F), glm::vec2(3.0F, 1.0F), white);
    canvas.rect(glm::vec2(4.0F, 0.0F), glm::vec2(5.0F, 1.0F), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white, atlas);

    BOOST_CHECK_EQUAL(canvas.batches().size(), 3);
}

/**
 * The transform applies as vertices are added, so a translate moves what is drawn after it
 * and a pop puts it back.
 **/
BOOST_AUTO_TEST_CASE(the_transform_stack_offsets_what_is_drawn_under_it) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.push();
    canvas.translate(glm::vec2(100.0F, 200.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);
    canvas.pop();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 100.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 200.0F, 0.001F);
    BOOST_CHECK_SMALL(canvas.vertices()[4].position.x, 0.001F);
    BOOST_CHECK_SMALL(canvas.vertices()[4].position.y, 0.001F);
}

/**
 * A pop with nothing pushed leaves the canvas's own identity in place, not an empty stack, so
 * an unbalanced pop in one caller does not corrupt every later draw.
 **/
BOOST_AUTO_TEST_CASE(an_unbalanced_pop_leaves_the_identity_transform) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.pop();
    canvas.pop();
    canvas.rect(glm::vec2(5.0F, 6.0F), glm::vec2(7.0F, 8.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 5.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 6.0F, 0.001F);
}

/**
 * clear() drops the geometry and the transform stack but keeps the size, since the canvas is
 * refilled every frame and resized only when the window is.
 **/
BOOST_AUTO_TEST_CASE(clearing_keeps_the_size_and_resets_the_transform) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    canvas.push();
    canvas.translate(glm::vec2(50.0F, 50.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.clear();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);

    BOOST_CHECK_EQUAL(canvas.width(), 800);
    BOOST_CHECK_EQUAL(canvas.height(), 600);
    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_SMALL(canvas.vertices()[0].position.x, 0.001F);
    BOOST_CHECK_SMALL(canvas.vertices()[0].position.y, 0.001F);
}

/**
 * The projection puts pixel (0,0) at the top left of clip space and the far corner at the
 * bottom right. Getting the y direction wrong draws the whole frame upside down.
 **/
BOOST_AUTO_TEST_CASE(the_projection_puts_the_origin_at_the_top_left) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(800, 600));

    const glm::vec2 topLeft = project(canvas, glm::vec2(0.0F, 0.0F));
    const glm::vec2 bottomRight = project(canvas, glm::vec2(800.0F, 600.0F));
    const glm::vec2 centre = project(canvas, glm::vec2(400.0F, 300.0F));

    BOOST_CHECK_CLOSE(topLeft.x, -1.0F, 0.001F);
    BOOST_CHECK_CLOSE(topLeft.y, -1.0F, 0.001F);
    BOOST_CHECK_CLOSE(bottomRight.x, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(bottomRight.y, 1.0F, 0.001F);
    BOOST_CHECK_SMALL(centre.x, 0.001F);
    BOOST_CHECK_SMALL(centre.y, 0.001F);
}

/**
 * A canvas that has never been sized still produces a usable projection, without dividing by
 * zero. A window can report no area before the first resize.
 **/
BOOST_AUTO_TEST_CASE(an_unsized_canvas_has_a_finite_projection) {
    const v3d::render::realtime::canvas::Canvas canvas;

    const glm::mat4 projection = canvas.projection();

    BOOST_CHECK(projection[0][0] > 0.0F);
    BOOST_CHECK(projection[1][1] > 0.0F);
}

/**
 * A circle is a fan of triangles, and it batches with the untextured quads around it.
 **/
BOOST_AUTO_TEST_CASE(a_circle_is_a_fan_that_batches_with_untextured_quads) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.circle(glm::vec2(50.0F, 50.0F), 10.0F, 8, white);

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6 + (8 * 3));
    // the centre, one vertex per side, and a repeat of the first so the wrap needs no case
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4 + 1 + 9);
}

/**
 * A ring is a strip between two radii, with two triangles per segment over a pair of vertices
 * at each end. It batches with the untextured quads around it.
 **/
BOOST_AUTO_TEST_CASE(a_ring_is_a_strip_between_two_radii) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.ring({.centre = glm::vec2(50.0F, 50.0F), .outer = 10.0F, .inner = 6.0F, .sides = 8, .start = 0.0F, .sweep = 6.283185307179586F}, white);

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6 + (8 * 6));
    // a pair per side, and a repeat of the first pair so the wrap needs no case
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4 + (9 * 2));

    // nothing lands in the hole the ring leaves
    for (std::size_t index = 4; index < canvas.vertices().size(); index++) {
        const glm::vec2 offset = glm::vec2(canvas.vertices()[index].position) - glm::vec2(50.0F, 50.0F);
        BOOST_CHECK_GE(std::sqrt((offset.x * offset.x) + (offset.y * offset.y)), 6.0F - 0.001F);
    }
}

/**
 * A ring with no hole is the wedge that fills one, so a caller need not test the radius it
 * was given before asking for a band.
 **/
BOOST_AUTO_TEST_CASE(a_ring_with_no_hole_is_a_wedge) {
    v3d::render::realtime::canvas::Canvas canvas;
    v3d::render::realtime::canvas::Canvas wedge;

    canvas.ring({.centre = glm::vec2(50.0F, 50.0F), .outer = 10.0F, .inner = 0.0F, .sides = 8, .start = 0.0F, .sweep = 1.5F}, white);
    wedge.arc({.centre = glm::vec2(50.0F, 50.0F), .radius = 10.0F, .sides = 8, .start = 0.0F, .sweep = 1.5F}, white);

    BOOST_CHECK_EQUAL(canvas.indices().size(), wedge.indices().size());
    BOOST_CHECK_EQUAL(canvas.vertices().size(), wedge.vertices().size());
}

/**
 * Too few sides to be a shape draws nothing rather than emitting degenerate triangles.
 **/
BOOST_AUTO_TEST_CASE(a_circle_of_fewer_than_three_sides_draws_nothing) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.circle(glm::vec2(0.0F, 0.0F), 10.0F, 2, white);

    BOOST_CHECK(canvas.empty());
}

/**
 * Laid out text is copied in against the atlas it was packed into, with its indices rebased
 * onto the canvas's own vertex stream.
 **/
BOOST_AUTO_TEST_CASE(text_is_appended_against_its_atlas_and_reindexed) {
    v3d::render::realtime::canvas::Canvas canvas;
    const v3d::render::realtime::TextureHandle atlas(4);

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);

    // one glyph quad, as a font would have laid it out
    std::vector<v3d::render::realtime::canvas::Canvas::Vertex> quad;
    for (int corner = 0; corner < 4; corner++) {
        quad.push_back({.position = glm::vec2(static_cast<float>(corner), 0.0F), .uv = glm::vec2(0.0F), .colour = white});
    }
    const std::vector<uint32_t> order = {0, 2, 1, 0, 3, 2};

    canvas.text(quad, order, atlas);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 2);
    BOOST_CHECK(canvas.batches()[1].texture == atlas);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 6);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 8);
    // the glyph's index 0 has to become index 4, since the rectangle is already in the stream
    BOOST_CHECK_EQUAL(canvas.indices()[6], 4);
}

/**
 * Text with nothing in it adds neither vertices nor a batch, so drawing an empty string does
 * not split the batch a caller is in the middle of.
 **/
BOOST_AUTO_TEST_CASE(empty_text_adds_no_batch) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.text({}, {}, v3d::render::realtime::TextureHandle(9));

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
}

/**
 * A run with an index past its own vertices draws nothing, rather than a triangle through a
 * vertex the run did not add.
 **/
BOOST_AUTO_TEST_CASE(text_with_an_index_past_its_vertices_draws_nothing) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    const std::vector<v3d::render::realtime::canvas::Canvas::Vertex> one = {
        {.position = glm::vec2(0.0F), .uv = glm::vec2(0.0F), .colour = white}};
    const std::vector<uint32_t> order = {0, 0, 1};
    canvas.text(one, order, v3d::render::realtime::TextureHandle(9));

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 4);
}

/**
 * A scale multiplies what is drawn under it, and a pop takes it back off. Something authored at
 * one size can then be drawn at another without scaling its own numbers.
 **/
BOOST_AUTO_TEST_CASE(a_scale_multiplies_what_is_drawn_under_it) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.push();
    canvas.translate(glm::vec2(10.0F, 20.0F));
    canvas.scale(glm::vec2(2.0F, 3.0F));
    canvas.rect(glm::vec2(1.0F, 1.0F), glm::vec2(2.0F, 2.0F), white);
    canvas.pop();
    canvas.rect(glm::vec2(1.0F, 1.0F), glm::vec2(2.0F, 2.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8);
    // scaled about the origin the transform was translated to, so the corner is 10 + 2
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.x, 12.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[0].position.y, 23.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.x, 14.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[2].position.y, 26.0F, 0.001F);
    // and the scale went away with the pop
    BOOST_CHECK_CLOSE(canvas.vertices()[4].position.x, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.vertices()[6].position.x, 2.0F, 0.001F);
}

/**
 * A clip starts a new batch where it opens and where it closes, and the batch between them
 * carries the rectangle the device scissors to. Nothing is dropped on the cpu: the quad
 * inside the clip is still four vertices.
 **/
BOOST_AUTO_TEST_CASE(a_clip_cuts_the_batch_and_carries_its_rectangle) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);
    canvas.clip(glm::vec2(4.0F, 5.0F), glm::vec2(20.0F, 30.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(100.0F, 100.0F), white);
    canvas.unclip();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 3);
    BOOST_CHECK(!canvas.batches()[0].clipped);
    BOOST_CHECK(canvas.batches()[1].clipped);
    BOOST_CHECK_CLOSE(canvas.batches()[1].clip.x, 4.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[1].clip.y, 5.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[1].clip.z, 20.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[1].clip.w, 30.0F, 0.001F);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 6);
    BOOST_CHECK(!canvas.batches()[2].clipped);
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 12);
}

/**
 * The rectangle is in the coordinates being drawn in, so the transform applies to it the
 * same way it applies to a vertex.
 **/
BOOST_AUTO_TEST_CASE(a_clip_is_transformed_like_a_vertex) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.push();
    canvas.translate(glm::vec2(100.0F, 200.0F));
    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.unclip();
    canvas.pop();

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.x, 100.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.y, 200.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.z, 110.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.w, 210.0F, 0.001F);
}

/**
 * A clip inside a clip can only take room away, so a child of a scrolled panel can clip
 * against its own box without knowing what is around it.
 **/
BOOST_AUTO_TEST_CASE(a_nested_clip_can_only_take_room_away) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.clip(glm::vec2(10.0F, 10.0F), glm::vec2(50.0F, 50.0F));
    canvas.clip(glm::vec2(0.0F, 20.0F), glm::vec2(100.0F, 30.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.unclip();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 2);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.x, 10.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.y, 20.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.z, 50.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.w, 30.0F, 0.001F);
    // the outer one is what is left after the inner is popped
    BOOST_CHECK_CLOSE(canvas.batches()[1].clip.w, 50.0F, 0.001F);
}

/**
 * Two clips that miss each other leave an empty rectangle rather than an inverted one. An
 * inverted rectangle would be a validation error once it reached a scissor.
 **/
BOOST_AUTO_TEST_CASE(clips_that_miss_each_other_leave_an_empty_rectangle) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F));
    canvas.clip(glm::vec2(40.0F, 40.0F), glm::vec2(50.0F, 50.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK(canvas.batches()[0].clip.z >= canvas.batches()[0].clip.x);
    BOOST_CHECK(canvas.batches()[0].clip.w >= canvas.batches()[0].clip.y);
    BOOST_CHECK_CLOSE(canvas.batches()[0].clip.z, canvas.batches()[0].clip.x, 0.001F);
}

/**
 * Going back to the same rectangle does not reopen a batch, so a run of siblings clipped to
 * one parent is one draw however many of them there are.
 **/
BOOST_AUTO_TEST_CASE(drawing_under_one_clip_stays_in_one_batch) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.rect(glm::vec2(2.0F, 2.0F), glm::vec2(3.0F, 3.0F), red);
    canvas.unclip();
    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F));
    canvas.rect(glm::vec2(4.0F, 4.0F), glm::vec2(5.0F, 5.0F), white);

    BOOST_CHECK_EQUAL(canvas.batches().size(), 1);
}

/**
 * An unbalanced unclip leaves the canvas unclipped, not with an empty stack, in the same way
 * that an unbalanced pop leaves the identity transform.
 **/
BOOST_AUTO_TEST_CASE(an_unbalanced_unclip_leaves_the_canvas_uncut) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.unclip();
    canvas.unclip();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK(!canvas.batches()[0].clipped);
}

/**
 * clear() drops what is clipped along with the transform, so a frame that threw between a
 * clip and its unclip does not clip the next one.
 **/
BOOST_AUTO_TEST_CASE(clearing_drops_what_is_clipped) {
    v3d::render::realtime::canvas::Canvas canvas;

    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);
    canvas.clear();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), white);

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK(!canvas.batches()[0].clipped);
}

/**
 * A space contained in a wider canvas is as large as fits, centred, with bars either side,
 * and its corners project to the viewport's.
 **/
BOOST_AUTO_TEST_CASE(a_contained_space_is_centred_with_bars) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(1600, 900));
    canvas.space(glm::vec2(800.0F, 600.0F), v3d::render::realtime::canvas::Canvas::Fit::Contain);

    const glm::vec4 area = canvas.viewport();
    BOOST_CHECK_CLOSE(area.x, 200.0F, 0.001F);
    BOOST_CHECK_SMALL(area.y, 0.001F);
    BOOST_CHECK_CLOSE(area.z, 1200.0F, 0.001F);
    BOOST_CHECK_CLOSE(area.w, 900.0F, 0.001F);

    // 200 of 1600 pixels in is a quarter of the way across clip space's two units
    const glm::vec2 topLeft = project(canvas, glm::vec2(0.0F, 0.0F));
    const glm::vec2 bottomRight = project(canvas, glm::vec2(800.0F, 600.0F));
    BOOST_CHECK_CLOSE(topLeft.x, -0.75F, 0.001F);
    BOOST_CHECK_CLOSE(topLeft.y, -1.0F, 0.001F);
    BOOST_CHECK_CLOSE(bottomRight.x, 0.75F, 0.001F);
    BOOST_CHECK_CLOSE(bottomRight.y, 1.0F, 0.001F);

    // and a resize moves the viewport, not the space
    canvas.resize(glm::uvec2(800, 1200));
    BOOST_CHECK_SMALL(canvas.viewport().x, 0.001F);
    BOOST_CHECK_CLOSE(canvas.viewport().y, 300.0F, 0.001F);
    BOOST_CHECK_CLOSE(canvas.space().x, 800.0F, 0.001F);
}

/**
 * A stretched space fills the canvas whatever its aspect.
 **/
BOOST_AUTO_TEST_CASE(a_stretched_space_fills_the_canvas) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(1600, 900));
    canvas.space(glm::vec2(800.0F, 600.0F), v3d::render::realtime::canvas::Canvas::Fit::Stretch);

    const glm::vec2 topLeft = project(canvas, glm::vec2(0.0F, 0.0F));
    const glm::vec2 bottomRight = project(canvas, glm::vec2(800.0F, 600.0F));
    BOOST_CHECK_CLOSE(topLeft.x, -1.0F, 0.001F);
    BOOST_CHECK_CLOSE(bottomRight.x, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(bottomRight.y, 1.0F, 0.001F);
}

/**
 * A cursor in the canvas's pixels comes back in the space's units, at the corners and the
 * centre, and outside the viewport it lands outside the space.
 **/
BOOST_AUTO_TEST_CASE(a_pixel_maps_back_into_the_space) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(1600, 900));
    canvas.space(glm::vec2(800.0F, 600.0F), v3d::render::realtime::canvas::Canvas::Fit::Contain);

    const glm::vec2 corner = canvas.toSpace(glm::vec2(200.0F, 0.0F));
    BOOST_CHECK_SMALL(corner.x, 0.001F);
    BOOST_CHECK_SMALL(corner.y, 0.001F);
    const glm::vec2 far = canvas.toSpace(glm::vec2(1400.0F, 900.0F));
    BOOST_CHECK_CLOSE(far.x, 800.0F, 0.001F);
    BOOST_CHECK_CLOSE(far.y, 600.0F, 0.001F);
    const glm::vec2 centre = canvas.toSpace(glm::vec2(800.0F, 450.0F));
    BOOST_CHECK_CLOSE(centre.x, 400.0F, 0.001F);
    BOOST_CHECK_CLOSE(centre.y, 300.0F, 0.001F);
    BOOST_CHECK(canvas.toSpace(glm::vec2(100.0F, 450.0F)).x < 0.0F);
}

/**
 * A canvas with no space projects in its own pixels over the whole window, and so does one
 * whose space was removed again.
 **/
BOOST_AUTO_TEST_CASE(a_canvas_without_a_space_projects_as_before) {
    v3d::render::realtime::canvas::Canvas plain;
    plain.resize(glm::uvec2(800, 600));
    v3d::render::realtime::canvas::Canvas cleared;
    cleared.resize(glm::uvec2(800, 600));
    cleared.space(glm::vec2(320.0F, 200.0F), v3d::render::realtime::canvas::Canvas::Fit::Contain);
    cleared.space(glm::vec2(0.0F, 0.0F), v3d::render::realtime::canvas::Canvas::Fit::Contain);

    BOOST_CHECK(plain.projection() == cleared.projection());
    BOOST_CHECK(cleared.toSpace(glm::vec2(12.0F, 34.0F)) == glm::vec2(12.0F, 34.0F));
    const glm::vec4 area = cleared.viewport();
    BOOST_CHECK(area == glm::vec4(0.0F, 0.0F, 800.0F, 600.0F));
}

/**
 * A clip is drawn in the space's units and scissored in pixels, so the rectangle a batch
 * carries is the one the viewport puts it at.
 **/
BOOST_AUTO_TEST_CASE(a_clip_in_a_space_is_cut_in_pixels) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(1600, 900));
    canvas.space(glm::vec2(800.0F, 600.0F), v3d::render::realtime::canvas::Canvas::Fit::Contain);

    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(400.0F, 300.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);
    canvas.unclip();

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    const glm::vec4 cut = canvas.batches()[0].clip;
    BOOST_CHECK_CLOSE(cut.x, 200.0F, 0.001F);
    BOOST_CHECK_SMALL(cut.y, 0.001F);
    BOOST_CHECK_CLOSE(cut.z, 800.0F, 0.001F);
    BOOST_CHECK_CLOSE(cut.w, 450.0F, 0.001F);
}

/**
 * A space survives the clear at the start of every frame, as the canvas's size does.
 **/
BOOST_AUTO_TEST_CASE(a_space_survives_a_clear) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(1600, 900));
    canvas.space(glm::vec2(800.0F, 600.0F), v3d::render::realtime::canvas::Canvas::Fit::Contain);
    canvas.clear();
    BOOST_CHECK_CLOSE(canvas.viewport().z, 1200.0F, 0.001F);
}

/**
 * A segment at an angle has its long sides parallel to the line between its points, half the
 * width either side of it, so its corners are offset across the line and not along an axis.
 **/
BOOST_AUTO_TEST_CASE(a_segment_at_an_angle_is_offset_across_its_line) {
    v3d::render::realtime::canvas::Canvas canvas;
    const float angle = 0.5235988F;  // thirty degrees
    const glm::vec2 from(10.0F, 20.0F);
    const glm::vec2 to = from + (glm::vec2(std::cos(angle), std::sin(angle)) * 40.0F);
    canvas.segment({.from = from, .to = to, .width = 4.0F}, red);

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 4);
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6);
    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 1);
    BOOST_CHECK(!canvas.batches()[0].texture.valid());

    // the line's direction turned a quarter, half the width long
    const glm::vec2 side = glm::vec2(-std::sin(angle), std::cos(angle)) * 2.0F;
    const glm::vec2 corners[] = {from - side, from + side, to + side, to - side};
    for (std::size_t index = 0; index < 4; index++) {
        BOOST_CHECK_CLOSE(canvas.vertices()[index].position.x, corners[index].x, 0.001F);
        BOOST_CHECK_CLOSE(canvas.vertices()[index].position.y, corners[index].y, 0.001F);
        BOOST_CHECK(canvas.vertices()[index].colour == red);
    }
}

/**
 * A segment is drawn through the transform stack as a rectangle is, so one laid along an axis
 * lands on the corners of the rectangle it covers.
 **/
BOOST_AUTO_TEST_CASE(a_segment_under_a_transform_lands_where_a_rectangle_would) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.push();
    canvas.translate(glm::vec2(10.0F, 20.0F));
    canvas.scale(glm::vec2(2.0F, 3.0F));
    canvas.rect(glm::vec2(1.0F, 1.0F), glm::vec2(2.0F, 2.0F), white);
    canvas.segment({.from = glm::vec2(1.0F, 1.5F), .to = glm::vec2(2.0F, 1.5F), .width = 1.0F}, white);
    canvas.pop();

    BOOST_REQUIRE_EQUAL(canvas.vertices().size(), 8);
    for (std::size_t index = 0; index < 4; index++) {
        BOOST_CHECK_CLOSE(canvas.vertices()[index + 4].position.x, canvas.vertices()[index].position.x, 0.001F);
        BOOST_CHECK_CLOSE(canvas.vertices()[index + 4].position.y, canvas.vertices()[index].position.y, 0.001F);
    }
}

/**
 * A segment is untextured, so it joins the open untextured batch rather than cutting a new
 * one, inside a clip as well as out of one.
 **/
BOOST_AUTO_TEST_CASE(a_segment_joins_the_untextured_batch) {
    v3d::render::realtime::canvas::Canvas canvas;
    const v3d::render::realtime::canvas::Canvas::Segment diagonal{
        .from = glm::vec2(0.0F, 0.0F), .to = glm::vec2(30.0F, 40.0F), .width = 2.0F};

    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);
    canvas.segment(diagonal, white);
    canvas.rect(glm::vec2(20.0F, 0.0F), glm::vec2(30.0F, 10.0F), white);
    canvas.clip(glm::vec2(0.0F, 0.0F), glm::vec2(50.0F, 50.0F));
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(10.0F, 10.0F), white);
    canvas.segment(diagonal, white);
    canvas.rect(glm::vec2(20.0F, 0.0F), glm::vec2(30.0F, 10.0F), white);
    canvas.unclip();

    BOOST_REQUIRE_EQUAL(canvas.batches().size(), 2);
    BOOST_CHECK(!canvas.batches()[0].clipped);
    BOOST_CHECK_EQUAL(canvas.batches()[0].indices, 18);
    BOOST_CHECK(canvas.batches()[1].clipped);
    BOOST_CHECK_EQUAL(canvas.batches()[1].indices, 18);
}

/**
 * A segment with no length or no width has no sides to draw, so it adds nothing, not even an
 * empty batch.
 **/
BOOST_AUTO_TEST_CASE(a_segment_with_no_length_or_width_draws_nothing) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.segment({.from = glm::vec2(5.0F, 5.0F), .to = glm::vec2(5.0F, 5.0F), .width = 3.0F}, white);
    canvas.segment({.from = glm::vec2(0.0F, 0.0F), .to = glm::vec2(5.0F, 5.0F), .width = 0.0F}, white);

    BOOST_CHECK(canvas.empty());
    BOOST_CHECK(canvas.batches().empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
