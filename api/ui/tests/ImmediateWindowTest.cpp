/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "Layer.h"

namespace v3d::ui::layer {
namespace {

BOOST_AUTO_TEST_SUITE(immediate_test)

/**
 * A window puts its contents inside itself, and folds to its title bar when the bar is
 * clicked. It has to be closed either way.
 **/
BOOST_AUTO_TEST_CASE(a_window_holds_its_contents_and_folds_away) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0F, 40.0F);
    const glm::vec2 size(200.0F, 150.0F);
    const glm::vec2 onTitle(60.0F, 45.0F);

    ui.begin(&canvas, hover(onTitle));
    BOOST_CHECK(ui.window("Encounter", {.position = corner, .size = size}, 1.0F));
    ui.text("turn 3");
    ui.endWindow();
    ui.end();

    // the title, then the line inside it, indented and below the bar
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_EQUAL(written[0].text, "Encounter");
    BOOST_CHECK(written[1].pen.x > corner.x);
    BOOST_CHECK(written[1].pen.y > corner.y + ui.dressing().barHeight);

    ui.begin(&canvas, press(onTitle));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();

    written.clear();
    ui.begin(&canvas, release(onTitle));
    BOOST_CHECK(!ui.window("Encounter", {.position = corner, .size = size}, 1.0F));
    ui.endWindow();
    ui.end();
    // folded: the title is still drawn and nothing else is
    BOOST_CHECK_EQUAL(written.size(), 1U);
}

/**
 * A window drawn later takes the cursor from one under it, because hover is decided at the
 * end of a frame.
 **/
BOOST_AUTO_TEST_CASE(the_window_drawn_last_takes_the_cursor) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 shared(60.0F, 75.0F);

    for (int frame = 0; frame < 2; frame++) {
        ui.begin(&canvas, frame == 0 ? hover(shared) : press(shared));
        ui.window("under", {.position = glm::vec2(40.0F, 40.0F), .size = glm::vec2(200.0F, 200.0F)}, 1.0F);
        ui.button("beneath");
        ui.endWindow();
        ui.window("over", {.position = glm::vec2(50.0F, 50.0F), .size = glm::vec2(200.0F, 200.0F)}, 1.0F);
        ui.endWindow();
        ui.end();
    }

    ui.begin(&canvas, release(shared));
    ui.window("under", {.position = glm::vec2(40.0F, 40.0F), .size = glm::vec2(200.0F, 200.0F)}, 1.0F);
    const bool beneath = ui.button("beneath");
    ui.endWindow();
    ui.window("over", {.position = glm::vec2(50.0F, 50.0F), .size = glm::vec2(200.0F, 200.0F)}, 1.0F);
    ui.endWindow();
    ui.end();

    BOOST_CHECK(!beneath);
}

/**
 * What a window holds is cut off at the window rather than drawn over what is beside it, so
 * the rows go into a batch carrying the window's body as its clip.
 **/
BOOST_AUTO_TEST_CASE(a_window_cuts_what_it_holds_off_at_its_edges) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0F, 40.0F);
    const glm::vec2 size(200.0F, 150.0F);

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    BOOST_REQUIRE(ui.window("Encounter", {.position = corner, .size = size}, 1.0F));
    ui.button("Go");
    ui.endWindow();
    ui.end();

    // the window's background and title bar are drawn before the clip opens, and what goes in
    // it after
    const std::vector<v3d::render::realtime::canvas::Canvas::Batch>& batches = canvas.batches();
    BOOST_REQUIRE(batches.size() >= 2U);
    BOOST_CHECK(!batches.front().clipped);
    const v3d::render::realtime::canvas::Canvas::Batch& cut = batches.back();
    BOOST_REQUIRE(cut.clipped);
    BOOST_CHECK_CLOSE(cut.clip.x, corner.x + ui.dressing().borderWidth, 0.001F);
    BOOST_CHECK_CLOSE(cut.clip.y, corner.y + ui.dressing().barHeight, 0.001F);
    BOOST_CHECK_CLOSE(cut.clip.z, corner.x + size.x - ui.dressing().borderWidth, 0.001F);
    BOOST_CHECK_CLOSE(cut.clip.w, corner.y + size.y - ui.dressing().borderWidth, 0.001F);
}

/**
 * A window that held more than it shows scrolls on the wheel, and the rows move up by what
 * it was scrolled by. The bar is decided from what the frame before drew, so the first
 * frame of an overflowing window still draws it unscrolled.
 **/
BOOST_AUTO_TEST_CASE(a_window_scrolls_when_it_holds_more_than_it_shows) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(20.0F, 20.0F);
    const glm::vec2 size(200.0F, 100.0F);
    const glm::vec2 inside(60.0F, 80.0F);

    // the frame that overflows, and turns the wheel
    ui.begin(&canvas, wheel(inside, -2.0F));
    BOOST_REQUIRE(ui.window("Log", {.position = corner, .size = size}, 1.0F));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();
    BOOST_REQUIRE_EQUAL(written.size(), 21U);
    const float before = written[1].pen.y;

    written.clear();
    ui.begin(&canvas, hover(inside));
    BOOST_REQUIRE(ui.window("Log", {.position = corner, .size = size}, 1.0F));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 21U);
    // two notches towards the reader, which is two notches further down the content
    BOOST_CHECK_CLOSE(before - written[1].pen.y, ui.dressing().lineHeight * 3.0F * 2.0F, 0.001F);
}

/**
 * The wheel turns the window the cursor is over and leaves the one beside it where it was.
 **/
BOOST_AUTO_TEST_CASE(the_wheel_turns_only_the_window_under_the_cursor) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 size(120.0F, 80.0F);
    const glm::vec2 left(10.0F, 10.0F);
    const glm::vec2 right(200.0F, 10.0F);

    for (int frame = 0; frame < 2; frame++) {
        written.clear();
        // the cursor is over the left window, and only the second frame has a bar to move
        ui.begin(&canvas, wheel(glm::vec2(40.0F, 60.0F), -1.0F));
        BOOST_REQUIRE(ui.window("Left", {.position = left, .size = size}, 1.0F));
        for (int row = 0; row < 12; row++) {
            ui.text("left");
        }
        ui.endWindow();
        BOOST_REQUIRE(ui.window("Right", {.position = right, .size = size}, 1.0F));
        for (int row = 0; row < 12; row++) {
            ui.text("right");
        }
        ui.endWindow();
        ui.end();
    }

    const float leftTop = written[1].pen.y;
    const float rightTop = written[14].pen.y;
    BOOST_CHECK_EQUAL(written[1].text, "left");
    BOOST_CHECK_EQUAL(written[14].text, "right");
    // the left one has been scrolled off its first row and the right one has not
    BOOST_CHECK(leftTop < rightTop);
}

/**
 * A bar appears on the frame after the one that overflowed, because how tall the content is
 * is only known once it has been drawn.
 **/
BOOST_AUTO_TEST_CASE(a_bar_appears_the_frame_after_a_window_overflows) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(20.0F, 20.0F);
    const glm::vec2 size(200.0F, 100.0F);

    // one long line, wrapped to whatever width the window leaves
    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    BOOST_REQUIRE(ui.window("Log", {.position = corner, .size = size}, 1.0F));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();
    const std::size_t unscrolled = canvas.indices().size();

    canvas.clear();
    written.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    BOOST_REQUIRE(ui.window("Log", {.position = corner, .size = size}, 1.0F));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();

    // the second frame has the first frame's overflow, so it draws a track and a thumb the first
    // had not
    BOOST_CHECK(canvas.indices().size() > unscrolled);
}

/**
 * A window put away for a moment comes back as it was left. Ageing out on the first frame a
 * widget is missing would lose the scroll and the fold of anything behind a toggle.
 **/
BOOST_AUTO_TEST_CASE(a_window_hidden_for_a_moment_keeps_what_it_held) {
    std::vector<Written> written;
    v3d::ui::Immediate ui = build(&written);
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));

    const glm::vec2 corner(10.0F, 10.0F);
    const glm::vec2 size(200.0F, 100.0F);

    // fold it by clicking its title bar, which is the only thing this layer folds one by.
    // Hover is a frame behind, so the cursor has to arrive before the press does
    const glm::vec2 onBar = corner + glm::vec2(size.x * 0.5F, 4.0F);
    bool folded = false;
    for (const v3d::ui::Immediate::Input& input : { hover(onBar), press(onBar), release(onBar) }) {
        ui.begin(&canvas, input);
        folded = !ui.window("tools", {.position = corner, .size = size}, 1.0F);
        ui.endWindow();
        ui.end();
        canvas.clear();
    }
    BOOST_CHECK(folded);

    // away for a few frames, and back
    for (unsigned int frame = 0; frame < 5; frame++) {
        ui.begin(&canvas, v3d::ui::Immediate::Input());
        ui.end();
        canvas.clear();
    }

    ui.begin(&canvas, v3d::ui::Immediate::Input());
    const bool stillFolded = !ui.window("tools", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();
    BOOST_CHECK(stillFolded);
}

/**
 * A press on a title bar that travels moves the window and does not fold it; one that stays
 * put folds it.
 **/
BOOST_AUTO_TEST_CASE(a_press_that_travels_moves_the_window_instead_of_folding_it) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0F, 40.0F);
    const glm::vec2 size(200.0F, 150.0F);
    const glm::vec2 onTitle(60.0F, 45.0F);
    const float travel = 40.0F;

    // the frame that finds the cursor, then the press that goes down on the bar
    ui.begin(&canvas, hover(onTitle));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();
    ui.begin(&canvas, press(onTitle));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();

    written.clear();
    ui.begin(&canvas, hold(onTitle + glm::vec2(travel, 0.0F)));
    BOOST_REQUIRE(ui.window("Encounter", {.position = corner, .size = size}, 1.0F));
    ui.endWindow();
    ui.end();

    // the window has moved by what the cursor did: the travel spent deciding is taken out
    // of the frame that decided rather than added to the window
    BOOST_REQUIRE(!written.empty());
    const float moved = written[0].pen.x - (corner.x + ui.dressing().padding);
    BOOST_CHECK_CLOSE(moved, travel, 0.001F);

    // and the release that ends the drag does not also fold it
    written.clear();
    ui.begin(&canvas, release(onTitle + glm::vec2(travel, 0.0F)));
    BOOST_CHECK(ui.window("Encounter", {.position = corner, .size = size}, 1.0F));
    ui.endWindow();
    ui.end();

    // still open, and still where it was dragged to
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_CLOSE(written[0].pen.x, corner.x + ui.dressing().padding + travel, 0.001F);
}

/**
 * A window cannot be dragged off the canvas, because its title bar is the only thing that
 * would drag it back.
 **/
BOOST_AUTO_TEST_CASE(a_dragged_window_keeps_its_title_bar_on_the_canvas) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0F, 40.0F);
    const glm::vec2 size(200.0F, 150.0F);
    const glm::vec2 onTitle(60.0F, 45.0F);

    ui.begin(&canvas, hover(onTitle));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();
    ui.begin(&canvas, press(onTitle));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();

    // far past the top left corner, and then far past the bottom right
    written.clear();
    ui.begin(&canvas, hold(glm::vec2(-500.0F, -500.0F)));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_CLOSE(written[0].pen.x, ui.dressing().padding, 0.001F);

    written.clear();
    ui.begin(&canvas, hold(glm::vec2(5000.0F, 5000.0F)));
    ui.window("Encounter", {.position = corner, .size = size}, 1.0F);
    ui.endWindow();
    ui.end();
    BOOST_REQUIRE(!written.empty());
    // the whole window fits across, so it stops with its right edge on the canvas. Down, it
    // stops with the bar on the canvas
    BOOST_CHECK_CLOSE(written[0].pen.x,
        static_cast<float>(canvas.width()) - size.x + ui.dressing().padding, 0.001F);
    BOOST_CHECK(written[0].pen.y < static_cast<float>(canvas.height()));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::layer
