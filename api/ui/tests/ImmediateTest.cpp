/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "Layer.h"

namespace v3d::ui::layer {
namespace {

BOOST_AUTO_TEST_SUITE(immediate_test)

/**
 * Rows stack down the canvas, and sameLine() puts the next one beside the last instead.
 **/
BOOST_AUTO_TEST_CASE(rows_stack_until_something_asks_to_stay_on_one) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.text("one");
    ui.text("two");
    ui.sameLine();
    ui.text("three");
    ui.text("four");
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 4U);
    BOOST_CHECK_CLOSE(written[0].pen.x, written[1].pen.x, 0.001F);
    BOOST_CHECK(written[1].pen.y > written[0].pen.y);
    // beside, and on the same row
    BOOST_CHECK(written[2].pen.x > written[1].pen.x);
    BOOST_CHECK_CLOSE(written[2].pen.y, written[1].pen.y, 0.001F);
    // and the row after it is one row further down, not two
    BOOST_CHECK_CLOSE(written[3].pen.y - written[1].pen.y, written[1].pen.y - written[0].pen.y, 0.001F);
}

/**
 * A button answers on the release that lands where its press did, and not before. Hover is
 * settled by the frame before, so the first frame a widget is drawn cannot click it.
 **/
BOOST_AUTO_TEST_CASE(a_button_answers_a_press_and_a_release_on_it) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 on(20.0F, 10.0F);

    // the frame that finds the cursor, which the next one uses
    ui.begin(&canvas, hover(on));
    BOOST_CHECK(!ui.button("Go"));
    ui.end();

    ui.begin(&canvas, press(on));
    BOOST_CHECK(!ui.button("Go"));
    ui.end();

    ui.begin(&canvas, release(on));
    BOOST_CHECK(ui.button("Go"));
    ui.end();

    // and the frame after the click is quiet again
    ui.begin(&canvas, hover(on));
    BOOST_CHECK(!ui.button("Go"));
    ui.end();
}

/**
 * A release somewhere other than where the press landed is not a click.
 **/
BOOST_AUTO_TEST_CASE(a_release_off_the_button_is_not_a_click) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(20.0F, 10.0F)));
    ui.button("Go");
    ui.end();
    ui.begin(&canvas, press(glm::vec2(20.0F, 10.0F)));
    ui.button("Go");
    ui.end();
    ui.begin(&canvas, release(glm::vec2(300.0F, 200.0F)));
    BOOST_CHECK(!ui.button("Go"));
    ui.end();
}

/**
 * A disabled widget neither answers nor lights up, and the scopes nest.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_widget_answers_nothing) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 on(20.0F, 10.0F);

    ui.begin(&canvas, hover(on));
    ui.button("Go");
    ui.end();
    ui.begin(&canvas, press(on));
    ui.button("Go");
    ui.end();

    ui.begin(&canvas, release(on));
    ui.beginDisabled();
    ui.beginDisabled();
    ui.endDisabled();
    BOOST_CHECK(!ui.button("Go"));
    ui.endDisabled();
    ui.end();

    // and it is drawn in the disabled colour while it is off
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK(written.back().colour == ui.dressing().disabledText);
}

/**
 * Two widgets with the same label in the same scope are one widget, so a press on one is a
 * press on the other and the release lands wherever the cursor went. pushId tells them
 * apart.
 **/
BOOST_AUTO_TEST_CASE(pushid_tells_two_widgets_of_one_label_apart) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 onFirst(20.0F, 10.0F);
    const glm::vec2 onSecond(20.0F, 36.0F);

    // pressed on the first and released on the second: sharing one id, the second reports a
    // click for a press it never had
    ui.begin(&canvas, hover(onFirst));
    ui.button("Kill");
    ui.button("Kill");
    ui.end();
    ui.begin(&canvas, press(onFirst));
    ui.button("Kill");
    ui.button("Kill");
    ui.end();
    ui.begin(&canvas, release(onSecond));
    const bool firstAnswered = ui.button("Kill");
    const bool secondAnswered = ui.button("Kill");
    ui.end();
    BOOST_CHECK(!firstAnswered);
    BOOST_CHECK(secondAnswered);

    // with an id of their own, the press stays on the button it landed on and the stray
    // release clicks nothing
    for (int frame = 0; frame < 2; frame++) {
        ui.begin(&canvas, frame == 0 ? hover(onFirst) : press(onFirst));
        ui.pushId(0);
        ui.button("Kill");
        ui.popId();
        ui.pushId(1);
        ui.button("Kill");
        ui.popId();
        ui.end();
    }
    ui.begin(&canvas, release(onSecond));
    ui.pushId(0);
    const bool first = ui.button("Kill");
    ui.popId();
    ui.pushId(1);
    const bool second = ui.button("Kill");
    ui.popId();
    ui.end();
    BOOST_CHECK(!first);
    BOOST_CHECK(!second);
}

/**
 * Wrapped text breaks at the spaces that do not fit, and a word too wide for the width is
 * left over the edge rather than split.
 **/
BOOST_AUTO_TEST_CASE(wrapped_text_breaks_at_a_space) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    // ten characters of room
    canvas.resize(glm::uvec2(100, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.textWrapped("aaa bbb ccc ddd");
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_EQUAL(written[0].text, "aaa bbb");
    BOOST_CHECK_EQUAL(written[1].text, "ccc ddd");

    written.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.textWrapped("aaaaaaaaaaaaaaaa");
    ui.end();
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
}

/**
 * A scrubbed int follows the cursor while it is held, and stops at the ends of its range.
 **/
BOOST_AUTO_TEST_CASE(a_scrubbed_int_follows_the_cursor_within_its_range) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    int value = 10;

    ui.begin(&canvas, hover(glm::vec2(20.0F, 10.0F)));
    ui.dragInt("hp", &value, 0, 20);
    ui.end();
    ui.begin(&canvas, press(glm::vec2(20.0F, 10.0F)));
    ui.dragInt("hp", &value, 0, 20);
    ui.end();

    v3d::ui::Immediate::Input held;
    held.cursor = glm::vec2(60.0F, 10.0F);
    held.down = true;
    ui.begin(&canvas, held);
    BOOST_CHECK(ui.dragInt("hp", &value, 0, 20));
    ui.end();
    BOOST_CHECK_EQUAL(value, 20);

    // and it does not run past the top
    held.cursor = glm::vec2(400.0F, 10.0F);
    ui.begin(&canvas, held);
    ui.dragInt("hp", &value, 0, 20);
    ui.end();
    BOOST_CHECK_EQUAL(value, 20);

    // the readout carries the value beside the label
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK(wrote(written, "hp  20"));
}

/**
 * A progress bar draws a track, the part of it that is filled, and its overlay. An empty one
 * draws no fill.
 **/
BOOST_AUTO_TEST_CASE(a_progress_bar_fills_what_it_was_given) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    ui.dressing().radius = 0.0F;

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.progressBar(0.0F, std::string());
    ui.end();
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6U);

    canvas.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.progressBar(0.5F, "half");
    ui.end();
    BOOST_CHECK_EQUAL(canvas.indices().size(), 2U * 6U);
    BOOST_CHECK(wrote(written, "half"));

    // everything the layer drew is untextured, so it is one batch
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1U);
}

/**
 * A widget that stops being drawn is aged out, so a panel whose ids come from changing text
 * costs what it drew recently rather than everything it has ever drawn.
 *
 * Without ageing out, the map of what the layer holds only grows.
 **/
BOOST_AUTO_TEST_CASE(what_stops_being_drawn_is_aged_out) {
    std::vector<Written> written;
    v3d::ui::Immediate ui = build(&written);
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));

    // a new window every frame, which is the churn the bound is for
    for (unsigned int frame = 0; frame < 400; frame++) {
        ui.begin(&canvas, v3d::ui::Immediate::Input());
        ui.window("panel " + std::to_string(frame), {.position = glm::vec2(10.0F, 10.0F), .size = glm::vec2(200.0F, 100.0F)}, 1.0F);
        ui.endWindow();
        ui.end();
        canvas.clear();
    }

    BOOST_CHECK_LE(ui.retained(), v3d::ui::Immediate::retention + 1);
}

/**
 * A widget told a width takes that much and no more, so two scrubbers fit one row.
 *
 * The failure this guards against is silent: a widget placed past the right edge is clipped
 * away and nothing reports it, so the second scrubber is simply not on screen.
 **/
BOOST_AUTO_TEST_CASE(a_widget_takes_the_width_it_was_given) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    int left = 1;
    int right = 2;

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.nextItemWidth(120.0F);
    ui.dragInt("left", &left, 0, 10);
    ui.sameLine();
    ui.nextItemWidth(120.0F);
    ui.dragInt("right", &right, 0, 10);
    ui.end();

    // both labels were drawn, and the second starts to the right of the first rather than
    // on the row below it
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_GT(written[1].pen.x, written[0].pen.x);
    BOOST_CHECK_EQUAL(written[1].pen.y, written[0].pen.y);
    // and neither ran off the canvas
    BOOST_CHECK_LT(written[1].pen.x, 400.0F);
}

/**
 * The width is spent by the widget after it and forgotten, so the one after that is back to
 * the rest of the row.
 **/
BOOST_AUTO_TEST_CASE(a_given_width_is_spent_once) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    int narrow = 1;
    int wide = 2;

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.nextItemWidth(80.0F);
    ui.dragInt("narrow", &narrow, 0, 10);
    ui.dragInt("wide", &wide, 0, 10);
    ui.end();

    // the second is on its own row and took all of it, so a press at the far right lands
    // on it and a press at the same x on the first row does not
    written.clear();
    ui.begin(&canvas, press(glm::vec2(300.0F, 8.0F)));
    ui.nextItemWidth(80.0F);
    ui.dragInt("narrow", &narrow, 0, 10);
    ui.dragInt("wide", &wide, 0, 10);
    ui.end();
    BOOST_CHECK(!ui.capturing());
}

/**
 * A blocked rectangle with nothing drawn in it captures the cursor the frame after, as a window
 * does, and only inside itself. Nothing is drawn there, so without the block the cursor is over
 * nothing this layer drew.
 **/
BOOST_AUTO_TEST_CASE(a_blocked_rectangle_captures_the_cursor_inside_it) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(200.0F, 150.0F)));
    ui.block(glm::vec2(100.0F, 100.0F), glm::vec2(300.0F, 200.0F));
    ui.end();
    BOOST_CHECK(ui.capturing());

    ui.begin(&canvas, hover(glm::vec2(50.0F, 250.0F)));
    ui.block(glm::vec2(100.0F, 100.0F), glm::vec2(300.0F, 200.0F));
    ui.end();
    BOOST_CHECK(!ui.capturing());
}

/**
 * A width wider than the room left is clamped to it rather than drawn off the edge.
 **/
BOOST_AUTO_TEST_CASE(a_given_width_cannot_exceed_the_row) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(200, 300));
    v3d::ui::Immediate ui = build(&written);
    const float fraction = 0.5F;

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    ui.nextItemWidth(10000.0F);
    ui.progressBar(fraction, "half");
    ui.end();

    // the overlay is centred in the bar, so a bar wider than the canvas would centre it
    // off the right edge
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_LT(written[0].pen.x, 200.0F);
}

/**
 * An app asks the layer whether a click has already been spent before it acts on one of its
 * own, as input::Cursor's return values do for the retained tree.
 **/
BOOST_AUTO_TEST_CASE(the_layer_says_when_it_wants_the_cursor) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    // nothing has been drawn, so nothing is wanted
    BOOST_CHECK(!ui.capturing());

    // a frame with the cursor inside a window claims the cursor for the next frame. An app
    // asks between the two
    ui.begin(&canvas, hover(glm::vec2(50.0F, 30.0F)));
    ui.window("panel", {.position = glm::vec2(10.0F, 10.0F), .size = glm::vec2(200.0F, 100.0F)}, 1.0F);
    ui.endWindow();
    ui.end();
    BOOST_CHECK(ui.capturing());

    // and a frame with the cursor outside it gives it back
    ui.begin(&canvas, hover(glm::vec2(350.0F, 250.0F)));
    ui.window("panel", {.position = glm::vec2(10.0F, 10.0F), .size = glm::vec2(200.0F, 100.0F)}, 1.0F);
    ui.endWindow();
    ui.end();
    BOOST_CHECK(!ui.capturing());
}

/**
 * The layer keeps the cursor through a drag that has left the widget, because the release that
 * ends the drag is still the layer's and not the scene's.
 **/
BOOST_AUTO_TEST_CASE(the_layer_keeps_the_cursor_through_a_drag) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    int value = 5;

    // a press only lands on a widget the frame before found the cursor on, so the hover
    // frame makes the press frame a press on this one
    ui.begin(&canvas, hover(glm::vec2(100.0F, 8.0F)));
    ui.dragInt("scrub", &value, 0, 10);
    ui.end();

    ui.begin(&canvas, press(glm::vec2(100.0F, 8.0F)));
    ui.dragInt("scrub", &value, 0, 10);
    ui.end();
    BOOST_REQUIRE(ui.capturing());

    // held, with the cursor dragged well clear of the widget
    v3d::ui::Immediate::Input held;
    held.cursor = glm::vec2(380.0F, 280.0F);
    held.down = true;
    ui.begin(&canvas, held);
    ui.dragInt("scrub", &value, 0, 10);
    ui.end();
    BOOST_CHECK(ui.capturing());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::layer
