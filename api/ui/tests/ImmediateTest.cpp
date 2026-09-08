/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../Immediate.h"

#include "../../render/realtime/Canvas.h"

namespace {

/**
 * A fixed width per character, so a widget's width is predictable.
 **/
const float characterWidth = 10.0f;

/**
 * What the layer asked to be written, so a test can read a panel back without a font, an
 * atlas or a device.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
};

/**
 * A layer that measures at ten pixels a character and records what it wrote.
 **/
v3d::ui::Immediate build(std::vector<Written>* written) {
    return v3d::ui::Immediate(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [written](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
            written->push_back(Written{std::string(text), pen, colour});
        });
}

/**
 * A press at a point, which is the first frame of a click.
 **/
v3d::ui::Immediate::Input press(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.down = true;
    input.pressed = true;
    return input;
}

/**
 * The release that finishes one.
 **/
v3d::ui::Immediate::Input release(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.released = true;
    return input;
}

/**
 * The cursor resting somewhere with nothing held.
 **/
v3d::ui::Immediate::Input hover(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    return input;
}

/**
 * The wheel turned, with the cursor resting where it is.
 **/
v3d::ui::Immediate::Input wheel(const glm::vec2& at, float notches) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.wheel = notches;
    return input;
}

/**
 * A press already down, resting where it has been dragged to.
 **/
v3d::ui::Immediate::Input hold(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.down = true;
    return input;
}

/**
 * @return whether anything written carried this string
 **/
bool wrote(const std::vector<Written>& written, const std::string& text) {
    return std::any_of(written.begin(), written.end(),
        [&text](const Written& line) { return line.text == text; });
}

};  // namespace

BOOST_AUTO_TEST_SUITE(immediate_test)

/**
 * Rows stack down the canvas, and sameLine() puts the next one beside the last instead.
 **/
BOOST_AUTO_TEST_CASE(rows_stack_until_something_asks_to_stay_on_one) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.text("one");
    ui.text("two");
    ui.sameLine();
    ui.text("three");
    ui.text("four");
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 4U);
    BOOST_CHECK_CLOSE(written[0].pen.x, written[1].pen.x, 0.001f);
    BOOST_CHECK(written[1].pen.y > written[0].pen.y);
    // beside, and on the same row
    BOOST_CHECK(written[2].pen.x > written[1].pen.x);
    BOOST_CHECK_CLOSE(written[2].pen.y, written[1].pen.y, 0.001f);
    // and the row after it is one row further down, not two
    BOOST_CHECK_CLOSE(written[3].pen.y - written[1].pen.y, written[1].pen.y - written[0].pen.y, 0.001f);
}

/**
 * A button answers on the release that lands where its press did, and not before. Hover is
 * settled by the frame before, so the first frame a widget is drawn cannot click it.
 **/
BOOST_AUTO_TEST_CASE(a_button_answers_a_press_and_a_release_on_it) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 on(20.0f, 10.0f);

    // the frame that finds the cursor, which is what the next one answers with
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
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(20.0f, 10.0f)));
    ui.button("Go");
    ui.end();
    ui.begin(&canvas, press(glm::vec2(20.0f, 10.0f)));
    ui.button("Go");
    ui.end();
    ui.begin(&canvas, release(glm::vec2(300.0f, 200.0f)));
    BOOST_CHECK(!ui.button("Go"));
    ui.end();
}

/**
 * A disabled widget neither answers nor lights up, and the scopes nest.
 **/
BOOST_AUTO_TEST_CASE(a_disabled_widget_answers_nothing) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 on(20.0f, 10.0f);

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

    // and it is drawn in the dim colour while it is off
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK(written.back().colour == ui.dressing().dimText);
}

/**
 * Two widgets with the same label in the same scope are one widget, so a press on one is a
 * press on the other and the release lands wherever the cursor went. pushId tells them
 * apart.
 **/
BOOST_AUTO_TEST_CASE(pushid_tells_two_widgets_of_one_label_apart) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 onFirst(20.0f, 10.0f);
    const glm::vec2 onSecond(20.0f, 36.0f);

    // pressed on the first and released on the second: sharing one id, the second answers
    // for a press it never had
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
    // release answers nothing
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
 * A window puts its contents inside itself, and folds to its title bar when the bar is
 * clicked. It has to be closed either way.
 **/
BOOST_AUTO_TEST_CASE(a_window_holds_its_contents_and_folds_away) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0f, 40.0f);
    const glm::vec2 size(200.0f, 150.0f);
    const glm::vec2 onTitle(60.0f, 45.0f);

    ui.begin(&canvas, hover(onTitle));
    BOOST_CHECK(ui.window("Encounter", corner, size, 1.0f));
    ui.text("turn 3");
    ui.endWindow();
    ui.end();

    // the title, then the line inside it, indented and below the bar
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_EQUAL(written[0].text, "Encounter");
    BOOST_CHECK(written[1].pen.x > corner.x);
    BOOST_CHECK(written[1].pen.y > corner.y + ui.dressing().barHeight);

    ui.begin(&canvas, press(onTitle));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();

    written.clear();
    ui.begin(&canvas, release(onTitle));
    BOOST_CHECK(!ui.window("Encounter", corner, size, 1.0f));
    ui.endWindow();
    ui.end();
    // folded: the title is still drawn and nothing else is
    BOOST_CHECK_EQUAL(written.size(), 1U);
}

/**
 * A tab strip takes a row of its own, so what a selected tab holds goes under the whole
 * strip. The selection is one of the few things kept between frames.
 **/
BOOST_AUTO_TEST_CASE(a_tab_strip_keeps_which_tab_is_selected) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    BOOST_REQUIRE(ui.tabBar("panels"));
    BOOST_CHECK(ui.tab("Turn"));
    BOOST_CHECK(!ui.tab("Entities"));
    ui.endTabBar();
    ui.end();

    // the second tab sits beside the first, not under it
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_CLOSE(written[0].pen.y, written[1].pen.y, 0.001f);
    BOOST_CHECK(written[1].pen.x > written[0].pen.x);

    const glm::vec2 onSecond(written[1].pen.x + 1.0f, written[1].pen.y - 2.0f);
    ui.begin(&canvas, hover(onSecond));
    ui.tabBar("panels");
    ui.tab("Turn");
    ui.tab("Entities");
    ui.endTabBar();
    ui.end();
    ui.begin(&canvas, press(onSecond));
    ui.tabBar("panels");
    ui.tab("Turn");
    ui.tab("Entities");
    ui.endTabBar();
    ui.end();
    // the frame that switches still answers for the tab that was selected when it began,
    // so only one tab draws what it holds
    ui.begin(&canvas, release(onSecond));
    ui.tabBar("panels");
    BOOST_CHECK(ui.tab("Turn"));
    BOOST_CHECK(!ui.tab("Entities"));
    ui.endTabBar();
    ui.end();

    ui.begin(&canvas, hover(onSecond));
    ui.tabBar("panels");
    const bool first = ui.tab("Turn");
    const bool second = ui.tab("Entities");
    ui.endTabBar();
    ui.end();

    BOOST_CHECK(!first);
    BOOST_CHECK(second);
}

/**
 * A table lays its cells out in columns, so the second cell of a row starts at the second
 * column rather than after the first cell's text.
 **/
BOOST_AUTO_TEST_CASE(a_table_puts_a_cell_in_its_column) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    BOOST_REQUIRE(ui.table("units", 2));
    ui.column("Name", 120.0f);
    ui.column("HP", 60.0f);
    ui.headerRow();
    ui.text("a");
    ui.nextColumn();
    ui.text("12");
    ui.nextRow();
    ui.text("b");
    ui.nextColumn();
    ui.text("7");
    ui.endTable();
    ui.text("after");
    ui.end();

    // two headers, then two cells a row
    BOOST_REQUIRE_EQUAL(written.size(), 7U);
    const float firstColumn = written[2].pen.x;
    const float secondColumn = written[3].pen.x;
    BOOST_CHECK(secondColumn > firstColumn + 100.0f);
    // the cells of one row share a baseline
    BOOST_CHECK_CLOSE(written[2].pen.y, written[3].pen.y, 0.001f);
    // and the next row is under both of them, back in the first column
    BOOST_CHECK_CLOSE(written[4].pen.x, firstColumn, 0.001f);
    BOOST_CHECK(written[4].pen.y > written[2].pen.y);
    BOOST_CHECK_CLOSE(written[5].pen.x, secondColumn, 0.001f);
    // what follows the table is back at the margin rather than in a column
    BOOST_CHECK_CLOSE(written[6].pen.x, firstColumn, 0.001f);
    BOOST_CHECK(written[6].pen.x < secondColumn);
}

/**
 * Wrapped text breaks at the spaces that do not fit, and a word too wide for the width is
 * left over the edge rather than split.
 **/
BOOST_AUTO_TEST_CASE(wrapped_text_breaks_at_a_space) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    // ten characters of room
    canvas.resize(100, 300);
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.textWrapped("aaa bbb ccc ddd");
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_EQUAL(written[0].text, "aaa bbb");
    BOOST_CHECK_EQUAL(written[1].text, "ccc ddd");

    written.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.textWrapped("aaaaaaaaaaaaaaaa");
    ui.end();
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
}

/**
 * A scrubbed int follows the cursor while it is held, and stops at the ends of its range.
 **/
BOOST_AUTO_TEST_CASE(a_scrubbed_int_follows_the_cursor_within_its_range) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    int value = 10;

    ui.begin(&canvas, hover(glm::vec2(20.0f, 10.0f)));
    ui.dragInt("hp", &value, 0, 20);
    ui.end();
    ui.begin(&canvas, press(glm::vec2(20.0f, 10.0f)));
    ui.dragInt("hp", &value, 0, 20);
    ui.end();

    v3d::ui::Immediate::Input held;
    held.cursor = glm::vec2(60.0f, 10.0f);
    held.down = true;
    ui.begin(&canvas, held);
    BOOST_CHECK(ui.dragInt("hp", &value, 0, 20));
    ui.end();
    BOOST_CHECK_EQUAL(value, 20);

    // and it does not run past the top
    held.cursor = glm::vec2(400.0f, 10.0f);
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
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    ui.dressing().radius = 0.0f;

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.progressBar(0.0f, std::string());
    ui.end();
    BOOST_CHECK_EQUAL(canvas.indices().size(), 6U);

    canvas.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.progressBar(0.5f, "half");
    ui.end();
    BOOST_CHECK_EQUAL(canvas.indices().size(), 2U * 6U);
    BOOST_CHECK(wrote(written, "half"));

    // everything the layer drew is untextured, so it is one batch
    BOOST_CHECK_EQUAL(canvas.batches().size(), 1U);
}

/**
 * A window drawn later takes the cursor from one under it, which is what deciding hover at
 * the end of a frame is for.
 **/
BOOST_AUTO_TEST_CASE(the_window_drawn_last_takes_the_cursor) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 shared(60.0f, 75.0f);

    for (int frame = 0; frame < 2; frame++) {
        ui.begin(&canvas, frame == 0 ? hover(shared) : press(shared));
        ui.window("under", glm::vec2(40.0f, 40.0f), glm::vec2(200.0f, 200.0f), 1.0f);
        ui.button("beneath");
        ui.endWindow();
        ui.window("over", glm::vec2(50.0f, 50.0f), glm::vec2(200.0f, 200.0f), 1.0f);
        ui.endWindow();
        ui.end();
    }

    ui.begin(&canvas, release(shared));
    ui.window("under", glm::vec2(40.0f, 40.0f), glm::vec2(200.0f, 200.0f), 1.0f);
    const bool beneath = ui.button("beneath");
    ui.endWindow();
    ui.window("over", glm::vec2(50.0f, 50.0f), glm::vec2(200.0f, 200.0f), 1.0f);
    ui.endWindow();
    ui.end();

    BOOST_CHECK(!beneath);
}

/**
 * What a window holds is cut off at the window rather than drawn over what is beside it, so
 * the rows go into a batch carrying the window's body - ADR-0037.
 **/
BOOST_AUTO_TEST_CASE(a_window_cuts_what_it_holds_off_at_its_edges) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0f, 40.0f);
    const glm::vec2 size(200.0f, 150.0f);

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    BOOST_REQUIRE(ui.window("Encounter", corner, size, 1.0f));
    ui.button("Go");
    ui.endWindow();
    ui.end();

    // the plate and the title bar are drawn before the clip opens, and what goes in it after
    const std::vector<v3d::render::realtime::Canvas::Batch>& batches = canvas.batches();
    BOOST_REQUIRE(batches.size() >= 2U);
    BOOST_CHECK(!batches.front().clipped);
    const v3d::render::realtime::Canvas::Batch& cut = batches.back();
    BOOST_REQUIRE(cut.clipped);
    BOOST_CHECK_CLOSE(cut.clip.x, corner.x + ui.dressing().borderWidth, 0.001f);
    BOOST_CHECK_CLOSE(cut.clip.y, corner.y + ui.dressing().barHeight, 0.001f);
    BOOST_CHECK_CLOSE(cut.clip.z, corner.x + size.x - ui.dressing().borderWidth, 0.001f);
    BOOST_CHECK_CLOSE(cut.clip.w, corner.y + size.y - ui.dressing().borderWidth, 0.001f);
}

/**
 * A window that held more than it shows scrolls on the wheel, and the rows move up by what
 * it was scrolled by. The bar is decided from what the frame before drew, so the first
 * frame of an overflowing window still draws it unscrolled.
 **/
BOOST_AUTO_TEST_CASE(a_window_scrolls_when_it_holds_more_than_it_shows) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(20.0f, 20.0f);
    const glm::vec2 size(200.0f, 100.0f);
    const glm::vec2 inside(60.0f, 80.0f);

    // the frame that overflows, and turns the wheel
    ui.begin(&canvas, wheel(inside, -2.0f));
    BOOST_REQUIRE(ui.window("Log", corner, size, 1.0f));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();
    BOOST_REQUIRE_EQUAL(written.size(), 21U);
    const float before = written[1].pen.y;

    written.clear();
    ui.begin(&canvas, hover(inside));
    BOOST_REQUIRE(ui.window("Log", corner, size, 1.0f));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 21U);
    // two notches towards the reader, which is two notches further down the content
    BOOST_CHECK_CLOSE(before - written[1].pen.y, ui.dressing().lineHeight * 3.0f * 2.0f, 0.001f);
}

/**
 * The wheel turns the window the cursor is over and leaves the one beside it where it was.
 **/
BOOST_AUTO_TEST_CASE(the_wheel_turns_only_the_window_under_the_cursor) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 size(120.0f, 80.0f);
    const glm::vec2 left(10.0f, 10.0f);
    const glm::vec2 right(200.0f, 10.0f);

    for (int frame = 0; frame < 2; frame++) {
        written.clear();
        // the cursor is over the left window, and only the second frame has a bar to move
        ui.begin(&canvas, wheel(glm::vec2(40.0f, 60.0f), -1.0f));
        BOOST_REQUIRE(ui.window("Left", left, size, 1.0f));
        for (int row = 0; row < 12; row++) {
            ui.text("left");
        }
        ui.endWindow();
        BOOST_REQUIRE(ui.window("Right", right, size, 1.0f));
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
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(20.0f, 20.0f);
    const glm::vec2 size(200.0f, 100.0f);

    // one long line, wrapped to whatever width the window leaves
    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    BOOST_REQUIRE(ui.window("Log", corner, size, 1.0f));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();
    const std::size_t unscrolled = canvas.indices().size();

    canvas.clear();
    written.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    BOOST_REQUIRE(ui.window("Log", corner, size, 1.0f));
    for (int row = 0; row < 20; row++) {
        ui.text("row");
    }
    ui.endWindow();
    ui.end();

    // the second frame knows it overflowed, so it draws a track and a thumb the first had not
    BOOST_CHECK(canvas.indices().size() > unscrolled);
}

/**
 * A widget that stops being drawn is aged out, so a panel whose ids come from changing text
 * costs what it drew recently rather than everything it has ever drawn.
 *
 * ADR-0035 named this as the bound on the layer's state. Without it the map only grows.
 **/
BOOST_AUTO_TEST_CASE(what_stops_being_drawn_is_aged_out) {
    std::vector<Written> written;
    v3d::ui::Immediate ui = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    // a new window every frame, which is the churn the bound is for
    for (unsigned int frame = 0; frame < 400; frame++) {
        ui.begin(&canvas, v3d::ui::Immediate::Input());
        ui.window("panel " + std::to_string(frame), glm::vec2(10.0f, 10.0f), glm::vec2(200.0f, 100.0f), 1.0f);
        ui.endWindow();
        ui.end();
        canvas.clear();
    }

    BOOST_CHECK_LE(ui.retained(), v3d::ui::Immediate::retention + 1);
}

/**
 * A window put away for a moment comes back as it was left. Ageing out on the first frame a
 * widget is missing would lose the scroll and the fold of anything behind a toggle.
 **/
BOOST_AUTO_TEST_CASE(a_window_hidden_for_a_moment_keeps_what_it_held) {
    std::vector<Written> written;
    v3d::ui::Immediate ui = build(&written);
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);

    const glm::vec2 corner(10.0f, 10.0f);
    const glm::vec2 size(200.0f, 100.0f);

    // fold it by clicking its title bar, which is the only thing this layer folds one by.
    // Hover is a frame behind, so the cursor has to arrive before the press does
    const glm::vec2 onBar = corner + glm::vec2(size.x * 0.5f, 4.0f);
    bool folded = false;
    for (const v3d::ui::Immediate::Input& input : { hover(onBar), press(onBar), release(onBar) }) {
        ui.begin(&canvas, input);
        folded = !ui.window("tools", corner, size, 1.0f);
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
    const bool stillFolded = !ui.window("tools", corner, size, 1.0f);
    ui.endWindow();
    ui.end();
    BOOST_CHECK(stillFolded);
}

/**
 * A widget told a width takes that much and no more, so two scrubbers fit one row.
 *
 * The failure this guards against is silent: a widget placed past the right edge is clipped
 * away and nothing reports it, so the second scrubber is simply not on screen.
 **/
BOOST_AUTO_TEST_CASE(a_widget_takes_the_width_it_was_given) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    int left = 1;
    int right = 2;

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.nextItemWidth(120.0f);
    ui.dragInt("left", &left, 0, 10);
    ui.sameLine();
    ui.nextItemWidth(120.0f);
    ui.dragInt("right", &right, 0, 10);
    ui.end();

    // both labels were drawn, and the second starts to the right of the first rather than
    // on the row below it
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_GT(written[1].pen.x, written[0].pen.x);
    BOOST_CHECK_EQUAL(written[1].pen.y, written[0].pen.y);
    // and neither ran off the canvas
    BOOST_CHECK_LT(written[1].pen.x, 400.0f);
}

/**
 * The width is spent by the widget after it and forgotten, so the one after that is back to
 * the rest of the row.
 **/
BOOST_AUTO_TEST_CASE(a_given_width_is_spent_once) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    int narrow = 1;
    int wide = 2;

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.nextItemWidth(80.0f);
    ui.dragInt("narrow", &narrow, 0, 10);
    ui.dragInt("wide", &wide, 0, 10);
    ui.end();

    // the second is on its own row and took all of it, so a press at the far right lands
    // on it and a press at the same x on the first row does not
    written.clear();
    ui.begin(&canvas, press(glm::vec2(300.0f, 8.0f)));
    ui.nextItemWidth(80.0f);
    ui.dragInt("narrow", &narrow, 0, 10);
    ui.dragInt("wide", &wide, 0, 10);
    ui.end();
    BOOST_CHECK(!ui.capturing());
}

/**
 * A width wider than the room left is clamped to it rather than drawn off the edge.
 **/
BOOST_AUTO_TEST_CASE(a_given_width_cannot_exceed_the_row) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(200, 300);
    v3d::ui::Immediate ui = build(&written);
    float fraction = 0.5f;

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    ui.nextItemWidth(10000.0f);
    ui.progressBar(fraction, "half");
    ui.end();

    // the overlay is centred in the bar, so a bar wider than the canvas would centre it
    // off the right edge
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_LT(written[0].pen.x, 200.0f);
}

/**
 * An app asks the layer whether a click has already been spent before it acts on one of its
 * own - the immediate half of the rule ADR-0038 states for the retained tree.
 **/
BOOST_AUTO_TEST_CASE(the_layer_says_when_it_wants_the_cursor) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);

    // nothing has been drawn, so nothing is wanted
    BOOST_CHECK(!ui.capturing());

    // a frame with the cursor inside a window claims it for the next one, which is when an
    // app asks
    ui.begin(&canvas, hover(glm::vec2(50.0f, 30.0f)));
    ui.window("panel", glm::vec2(10.0f, 10.0f), glm::vec2(200.0f, 100.0f), 1.0f);
    ui.endWindow();
    ui.end();
    BOOST_CHECK(ui.capturing());

    // and a frame with the cursor outside it gives it back
    ui.begin(&canvas, hover(glm::vec2(350.0f, 250.0f)));
    ui.window("panel", glm::vec2(10.0f, 10.0f), glm::vec2(200.0f, 100.0f), 1.0f);
    ui.endWindow();
    ui.end();
    BOOST_CHECK(!ui.capturing());
}

/**
 * And it keeps wanting it through a drag that has left the widget, because the release that
 * ends the drag is still the layer's and not the scene's.
 **/
BOOST_AUTO_TEST_CASE(the_layer_keeps_the_cursor_through_a_drag) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    int value = 5;

    // a press only lands on a widget the frame before found the cursor on, so the hover
    // frame is what makes the press frame a press on this one
    ui.begin(&canvas, hover(glm::vec2(100.0f, 8.0f)));
    ui.dragInt("scrub", &value, 0, 10);
    ui.end();

    ui.begin(&canvas, press(glm::vec2(100.0f, 8.0f)));
    ui.dragInt("scrub", &value, 0, 10);
    ui.end();
    BOOST_REQUIRE(ui.capturing());

    // held, with the cursor dragged well clear of the widget
    v3d::ui::Immediate::Input held;
    held.cursor = glm::vec2(380.0f, 280.0f);
    held.down = true;
    ui.begin(&canvas, held);
    ui.dragInt("scrub", &value, 0, 10);
    ui.end();
    BOOST_CHECK(ui.capturing());
}

/**
 * A press on a title bar that travels moves the window and does not fold it; one that stays
 * put folds it. ADR-0045.
 **/
BOOST_AUTO_TEST_CASE(a_press_that_travels_moves_the_window_instead_of_folding_it) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0f, 40.0f);
    const glm::vec2 size(200.0f, 150.0f);
    const glm::vec2 onTitle(60.0f, 45.0f);
    const float travel = 40.0f;

    // the frame that finds the cursor, then the press that goes down on the bar
    ui.begin(&canvas, hover(onTitle));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();
    ui.begin(&canvas, press(onTitle));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();

    written.clear();
    ui.begin(&canvas, hold(onTitle + glm::vec2(travel, 0.0f)));
    BOOST_REQUIRE(ui.window("Encounter", corner, size, 1.0f));
    ui.endWindow();
    ui.end();

    // the window has moved by what the cursor did: the travel spent deciding is taken out
    // of the frame that decided rather than added to the window
    BOOST_REQUIRE(!written.empty());
    const float moved = written[0].pen.x - (corner.x + ui.dressing().padding);
    BOOST_CHECK_CLOSE(moved, travel, 0.001f);

    // and the release that ends the drag does not also fold it
    written.clear();
    ui.begin(&canvas, release(onTitle + glm::vec2(travel, 0.0f)));
    BOOST_CHECK(ui.window("Encounter", corner, size, 1.0f));
    ui.endWindow();
    ui.end();

    // still open, and still where it was dragged to
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_CLOSE(written[0].pen.x, corner.x + ui.dressing().padding + travel, 0.001f);
}

/**
 * A window cannot be dragged off the canvas, because its title bar is the only thing that
 * would drag it back.
 **/
BOOST_AUTO_TEST_CASE(a_dragged_window_keeps_its_title_bar_on_the_canvas) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0f, 40.0f);
    const glm::vec2 size(200.0f, 150.0f);
    const glm::vec2 onTitle(60.0f, 45.0f);

    ui.begin(&canvas, hover(onTitle));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();
    ui.begin(&canvas, press(onTitle));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();

    // far past the top left corner, and then far past the bottom right
    written.clear();
    ui.begin(&canvas, hold(glm::vec2(-500.0f, -500.0f)));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();
    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_CLOSE(written[0].pen.x, ui.dressing().padding, 0.001f);

    written.clear();
    ui.begin(&canvas, hold(glm::vec2(5000.0f, 5000.0f)));
    ui.window("Encounter", corner, size, 1.0f);
    ui.endWindow();
    ui.end();
    BOOST_REQUIRE(!written.empty());
    // the whole window fits across, so it stops with its right edge on the canvas; down,
    // it stops with the bar on it
    BOOST_CHECK_CLOSE(written[0].pen.x,
        static_cast<float>(canvas.width()) - size.x + ui.dressing().padding, 0.001f);
    BOOST_CHECK(written[0].pen.y < static_cast<float>(canvas.height()));
}

/**
 * A table given a height scrolls its rows inside it and leaves its header above them, per
 * ADR-0046.
 **/
BOOST_AUTO_TEST_CASE(a_table_given_a_height_keeps_its_header_above_its_rows) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 over(50.0f, 60.0f);

    const auto roster = [&ui]() {
        BOOST_REQUIRE(ui.table("units", 2, 100.0f));
        ui.column("Name", 120.0f);
        ui.column("HP", 60.0f);
        ui.headerRow();
        for (int row = 0; row < 20; row++) {
            if (row > 0) {
                ui.nextRow();
            }
            ui.text("unit");
            ui.nextColumn();
            ui.text("hp");
        }
        ui.endTable();
    };

    // the frame that turns the wheel over the table
    ui.begin(&canvas, wheel(over, -2.0f));
    roster();
    ui.end();
    BOOST_REQUIRE_EQUAL(written.size(), 42U);
    const float header = written[0].pen.y;
    const float firstRow = written[2].pen.y;

    written.clear();
    ui.begin(&canvas, hover(over));
    roster();
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 42U);
    // the header has not moved and the rows have, which is the whole of the feature
    BOOST_CHECK_CLOSE(written[0].pen.y, header, 0.001f);
    BOOST_CHECK_CLOSE(firstRow - written[2].pen.y, ui.dressing().lineHeight * 3.0f * 2.0f, 0.001f);
}

/**
 * A table given a height is that tall whatever it holds, so what follows it does not move
 * when a row arrives.
 **/
BOOST_AUTO_TEST_CASE(a_table_given_a_height_is_that_tall_whatever_it_holds) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);

    const auto roster = [&ui](int rows) {
        BOOST_REQUIRE(ui.table("units", 1, 100.0f));
        ui.column("Name", 0.0f);
        ui.headerRow();
        for (int row = 0; row < rows; row++) {
            if (row > 0) {
                ui.nextRow();
            }
            ui.text("unit");
        }
        ui.endTable();
        ui.text("after");
    };

    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    roster(3);
    ui.end();
    BOOST_REQUIRE(!written.empty());
    const float few = written.back().pen.y;

    written.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0f, -1.0f)));
    roster(40);
    ui.end();

    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_CLOSE(written.back().pen.y, few, 0.001f);
}

/**
 * The wheel turns the innermost region under the cursor, so a table inside a window takes
 * it from the window rather than turning both.
 **/
BOOST_AUTO_TEST_CASE(the_wheel_turns_a_table_rather_than_the_window_holding_it) {
    std::vector<Written> written;
    v3d::render::realtime::Canvas canvas;
    canvas.resize(400, 300);
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(20.0f, 20.0f);
    const glm::vec2 size(220.0f, 160.0f);
    const glm::vec2 overTable(60.0f, 100.0f);

    const auto panel = [&ui, &corner, &size]() {
        BOOST_REQUIRE(ui.window("Roster", corner, size, 1.0f));
        ui.text("above");
        BOOST_REQUIRE(ui.table("units", 1, 80.0f));
        ui.column("Name", 0.0f);
        ui.headerRow();
        for (int row = 0; row < 20; row++) {
            if (row > 0) {
                ui.nextRow();
            }
            ui.text("unit");
        }
        ui.endTable();
        for (int line = 0; line < 10; line++) {
            ui.text("below");
        }
        ui.endWindow();
    };

    // a frame that measures, then the one that turns the wheel over the table
    ui.begin(&canvas, hover(overTable));
    panel();
    ui.end();
    written.clear();
    ui.begin(&canvas, wheel(overTable, -2.0f));
    panel();
    ui.end();
    BOOST_REQUIRE(written.size() > 3U);
    const float above = written[1].pen.y;
    const float header = written[2].pen.y;
    const float firstRow = written[3].pen.y;

    written.clear();
    ui.begin(&canvas, hover(overTable));
    panel();
    ui.end();

    BOOST_REQUIRE(written.size() > 3U);
    // the window kept its place, so what is above the table and the header are where they
    // were, and only the rows moved
    BOOST_CHECK_CLOSE(written[1].pen.y, above, 0.001f);
    BOOST_CHECK_CLOSE(written[2].pen.y, header, 0.001f);
    BOOST_CHECK(written[3].pen.y < firstRow);
}

BOOST_AUTO_TEST_SUITE_END()
