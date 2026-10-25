/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Immediate.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * A fixed width per character, so where a press lands in a line is predictable.
 **/
const float characterWidth = 10.0F;

/**
 * Where a field's line starts: the default padding in from the widget's left edge, which is
 * at the canvas's left edge outside any window.
 **/
const float lineStart = 6.0F;

/**
 * A point inside a widget drawn first on the canvas, which is the top row.
 **/
const glm::vec2 firstRow(100.0F, 10.0F);

/**
 * A point well away from anything drawn.
 **/
const glm::vec2 nowhere(390.0F, 290.0F);

/**
 * A layer that measures at ten pixels a character and writes nothing.
 **/
v3d::ui::Immediate build() {
    return v3d::ui::Immediate(
        [](std::string_view text) { return static_cast<float>(text.size()) * characterWidth; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
}

v3d::ui::Immediate::Input press(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.down = true;
    input.pressed = true;
    return input;
}

v3d::ui::Immediate::Input release(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.released = true;
    return input;
}

v3d::ui::Immediate::Input hover(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    return input;
}

v3d::ui::Immediate::Input hold(const glm::vec2& at) {
    v3d::ui::Immediate::Input input;
    input.cursor = at;
    input.down = true;
    return input;
}

/**
 * Draw one frame of a panel under some input.
 **/
void frame(v3d::ui::Immediate* ui, v3d::render::realtime::canvas::Canvas* canvas,
    const v3d::ui::Immediate::Input& input, const std::function<void()>& panel) {
    ui->begin(canvas, input);
    panel();
    ui->end();
}

/**
 * A click: the frame that finds the cursor, the press, and the release.
 **/
void click(v3d::ui::Immediate* ui, v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& at,
    const std::function<void()>& panel) {
    frame(ui, canvas, hover(at), panel);
    frame(ui, canvas, press(at), panel);
    frame(ui, canvas, release(at), panel);
}

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

/**
 * Of two widgets, only the one under the cursor answers that it is hovered, and text answers
 * as a widget does.
 **/
BOOST_AUTO_TEST_CASE(only_the_item_under_the_cursor_is_hovered) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    bool first = true;
    bool second = false;
    bool line = false;
    const auto panel = [&]() {
        ui.button("First");
        first = ui.itemHovered();
        ui.sameLine();
        ui.button("Second");
        second = ui.itemHovered();
        ui.text("a cost");
        line = ui.itemHovered();
    };
    // the second button starts past the first, which is "First" and its padding wide
    const glm::vec2 onSecond(80.0F, 10.0F);

    frame(&ui, &canvas, hover(onSecond), panel);
    frame(&ui, &canvas, hover(onSecond), panel);
    BOOST_CHECK(!first);
    BOOST_CHECK(second);
    BOOST_CHECK(!line);

    // the text is on the row under the buttons
    const glm::vec2 onText(20.0F, 35.0F);
    frame(&ui, &canvas, hover(onText), panel);
    frame(&ui, &canvas, hover(onText), panel);
    BOOST_CHECK(!first);
    BOOST_CHECK(!second);
    BOOST_CHECK(line);
}

/**
 * A widget under a window drawn over it is not hovered, though the cursor is in its box.
 **/
BOOST_AUTO_TEST_CASE(an_item_under_a_window_is_not_hovered) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    bool under = true;
    const auto panel = [&]() {
        ui.button("Under");
        under = ui.itemHovered();
        ui.window("Over", {.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(200.0F, 100.0F)}, 1.0F);
        ui.endWindow();
    };

    frame(&ui, &canvas, hover(glm::vec2(20.0F, 10.0F)), panel);
    frame(&ui, &canvas, hover(glm::vec2(20.0F, 10.0F)), panel);
    BOOST_CHECK(!under);
}

/**
 * A click flips a checkbox, and only the frame of the click reports it.
 **/
BOOST_AUTO_TEST_CASE(a_click_flips_a_checkbox) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    bool vsync = false;
    bool changed = false;
    const auto panel = [&]() { changed = ui.checkbox("Vsync", &vsync); };
    const glm::vec2 onBox(5.0F, 5.0F);

    click(&ui, &canvas, onBox, panel);
    BOOST_CHECK(vsync);
    BOOST_CHECK(changed);

    frame(&ui, &canvas, hover(onBox), panel);
    BOOST_CHECK(!changed);

    click(&ui, &canvas, onBox, panel);
    BOOST_CHECK(!vsync);
}

/**
 * A press on a slider's track sets the value to where it landed, a drag follows the cursor,
 * and a drag past the end holds the value at the end.
 **/
BOOST_AUTO_TEST_CASE(a_slider_follows_the_cursor_along_its_track) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    float volume = 0.5F;
    bool changed = false;
    const auto panel = [&]() { changed = ui.sliderFloat("Music", &volume, 0.0F, 2.0F); };

    // the track spans the canvas, so a quarter of the way along is a quarter of the range
    frame(&ui, &canvas, hover(glm::vec2(100.0F, 10.0F)), panel);
    frame(&ui, &canvas, press(glm::vec2(100.0F, 10.0F)), panel);
    BOOST_CHECK_CLOSE(volume, 0.5F, 0.001F);
    BOOST_CHECK(!changed);

    frame(&ui, &canvas, hold(glm::vec2(300.0F, 10.0F)), panel);
    BOOST_CHECK_CLOSE(volume, 1.5F, 0.001F);
    BOOST_CHECK(changed);

    // off the end of the track and off the widget, still held
    frame(&ui, &canvas, hold(glm::vec2(500.0F, 80.0F)), panel);
    BOOST_CHECK_CLOSE(volume, 2.0F, 0.001F);

    frame(&ui, &canvas, release(glm::vec2(500.0F, 80.0F)), panel);
    frame(&ui, &canvas, hover(glm::vec2(100.0F, 10.0F)), panel);
    BOOST_CHECK_CLOSE(volume, 2.0F, 0.001F);
}

/**
 * A click opens a combo's list, and a click on an item chooses it on the next frame. The list
 * is drawn over what follows the combo, so the click does not also reach a button under it.
 **/
BOOST_AUTO_TEST_CASE(a_combo_chooses_from_its_list_over_what_is_under_it) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    const std::vector<std::string> sizes = {"1280x720", "1920x1080", "2560x1440"};
    int chosen = 2;
    bool changed = false;
    bool pressed = false;
    const auto panel = [&]() {
        changed = ui.combo("Resolution", &chosen, sizes);
        pressed = ui.button("Apply") || pressed;
    };

    click(&ui, &canvas, firstRow, panel);
    // the combo is a bar tall, and its list starts under it, a line per item. The first item
    // covers the button drawn on the row under the combo
    const glm::vec2 onFirst(20.0F, 35.0F);
    click(&ui, &canvas, onFirst, panel);
    BOOST_CHECK(!pressed);
    BOOST_CHECK_EQUAL(chosen, 2);

    frame(&ui, &canvas, hover(nowhere), panel);
    BOOST_CHECK(changed);
    BOOST_CHECK_EQUAL(chosen, 0);

    // closed now, so the same click reaches the button
    click(&ui, &canvas, onFirst, panel);
    BOOST_CHECK(pressed);
    BOOST_CHECK_EQUAL(chosen, 0);
}

/**
 * A press outside an open combo's list closes it without choosing anything.
 **/
BOOST_AUTO_TEST_CASE(a_press_outside_a_combo_closes_it) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    const std::vector<std::string> sizes = {"small", "large"};
    int chosen = 1;
    bool pressed = false;
    const auto panel = [&]() {
        ui.combo("Size", &chosen, sizes);
        pressed = ui.button("Apply") || pressed;
    };
    // the first item, which covers the button while the list is open
    const glm::vec2 onItem(20.0F, 35.0F);

    click(&ui, &canvas, firstRow, panel);
    frame(&ui, &canvas, press(nowhere), panel);
    frame(&ui, &canvas, release(nowhere), panel);

    // the list is gone, so a click where its first item was reaches the button
    click(&ui, &canvas, onItem, panel);
    BOOST_CHECK(pressed);
    BOOST_CHECK_EQUAL(chosen, 1);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
