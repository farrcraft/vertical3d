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

BOOST_AUTO_TEST_SUITE(immediate_input_test)

/**
 * Characters and keys reach the field that holds the keyboard, between frames, and land in
 * the caller's string when the field is next drawn.
 **/
BOOST_AUTO_TEST_CASE(typing_edits_the_callers_string) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name = "slot";
    bool changed = false;
    const auto panel = [&]() { changed = ui.textField("Name", &name); };

    click(&ui, &canvas, firstRow, panel);
    BOOST_REQUIRE(ui.typing());

    BOOST_CHECK(ui.typed(" 1"));
    BOOST_CHECK(ui.press("arrow_left"));
    BOOST_CHECK(ui.press("backspace"));
    // nothing reaches the caller's string until the field is drawn
    BOOST_CHECK_EQUAL(name, "slot");

    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK_EQUAL(name, "slot1");
    BOOST_CHECK(changed);

    // and a frame with nothing typed changes nothing
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK(!changed);
}

/**
 * A field that holds the keyboard takes every key that composes text, so a letter typed into
 * it does not also fire a binding. Without one, nothing is taken.
 **/
BOOST_AUTO_TEST_CASE(a_field_takes_keys_only_while_it_holds_the_keyboard) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name;
    const auto panel = [&]() { ui.textField("Name", &name); };

    frame(&ui, &canvas, hover(nowhere), panel);
    BOOST_CHECK(!ui.typing());
    BOOST_CHECK(!ui.press("e"));
    BOOST_CHECK(!ui.typed("e"));

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(ui.press("e"));
    BOOST_CHECK(ui.typed("e"));
    // a key that composes nothing is left for the app
    BOOST_CHECK(!ui.press("f1"));
    // as is a chord that is not an edit, so a save still saves
    BOOST_CHECK(!ui.press("s", false, true));
}

/**
 * A return gives the keyboard up and keeps what was typed. The field writes it, and every key
 * after the return goes to the app.
 **/
BOOST_AUTO_TEST_CASE(a_return_gives_the_keyboard_up_keeping_the_text) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name;
    const auto panel = [&]() { ui.textField("Name", &name); };

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(ui.typed("abc"));
    BOOST_CHECK(ui.press("return"));
    BOOST_CHECK(!ui.typing());
    BOOST_CHECK(!ui.press("e"));

    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK_EQUAL(name, "abc");
    BOOST_CHECK(!ui.typing());
}

/**
 * A press anywhere but the field gives the keyboard up, and the text typed before it stays.
 **/
BOOST_AUTO_TEST_CASE(a_press_elsewhere_gives_the_keyboard_up) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name;
    const auto panel = [&]() { ui.textField("Name", &name); };

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(ui.typed("abc"));
    frame(&ui, &canvas, press(nowhere), panel);

    BOOST_CHECK(!ui.typing());
    BOOST_CHECK_EQUAL(name, "abc");
}

/**
 * A field that is not drawn cannot be typed into, so it gives the keyboard up at the end of
 * the frame, and whoever listens is told both times.
 **/
BOOST_AUTO_TEST_CASE(a_field_that_is_not_drawn_gives_the_keyboard_up) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::vector<bool> told;
    ui.onTyping([&told](bool typing) { told.push_back(typing); });
    std::string name;
    const auto panel = [&]() { ui.textField("Name", &name); };

    click(&ui, &canvas, firstRow, panel);
    BOOST_REQUIRE(ui.typing());
    frame(&ui, &canvas, hover(firstRow), []() {});

    BOOST_CHECK(!ui.typing());
    BOOST_REQUIRE_EQUAL(told.size(), 2U);
    BOOST_CHECK(told[0]);
    BOOST_CHECK(!told[1]);
}

/**
 * The press that gives a field the keyboard puts the caret where it landed, between the two
 * characters either side of it.
 **/
BOOST_AUTO_TEST_CASE(the_press_puts_the_caret_where_it_landed) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name = "abcd";
    const auto panel = [&]() { ui.textField("Name", &name); };

    // two characters and a little into the third, which is short of its middle
    const glm::vec2 between(lineStart + (characterWidth * 2.0F) + 2.0F, firstRow.y);
    click(&ui, &canvas, between, panel);
    BOOST_CHECK(ui.typed("X"));
    frame(&ui, &canvas, hover(between), panel);

    BOOST_CHECK_EQUAL(name, "abXcd");
}

/**
 * Typing past the limit is refused, and the refusal is still taken from the app.
 **/
BOOST_AUTO_TEST_CASE(typing_past_the_limit_is_refused) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name = "ab";
    const auto panel = [&]() { ui.textField("Name", &name, 3); };

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(ui.typed("c"));
    BOOST_CHECK(ui.typed("d"));
    frame(&ui, &canvas, hover(firstRow), panel);

    BOOST_CHECK_EQUAL(name, "abc");
}

/**
 * The caller's string is the value. A change the caller makes while the field is being
 * edited replaces what was being edited.
 **/
BOOST_AUTO_TEST_CASE(a_change_by_the_caller_replaces_the_edit) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name = "old";
    const auto panel = [&]() { ui.textField("Name", &name); };

    click(&ui, &canvas, firstRow, panel);
    name = "new";
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK(ui.typed("!"));
    frame(&ui, &canvas, hover(firstRow), panel);

    BOOST_CHECK_EQUAL(name, "new!");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(immediate_test)

/**
 * An empty field shows its label in the placeholder colour, which a theme names apart from
 * the colour of disabled text.
 **/
BOOST_AUTO_TEST_CASE(an_empty_field_shows_its_label_as_a_placeholder) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec4 hint(1.0F, 0.0F, 0.0F, 1.0F);
    ui.dressing().placeholder = hint;

    std::string value;
    ui.begin(&canvas, v3d::ui::Immediate::Input());
    ui.textField("Name", &value);
    ui.end();

    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_EQUAL(written.back().text, "Name");
    BOOST_CHECK(written.back().colour == hint);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::layer
