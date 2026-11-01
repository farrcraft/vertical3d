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

BOOST_AUTO_TEST_SUITE(immediate_input_test)

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
 * A surface says where the cursor is and what its press did: it goes down on the surface, stays
 * held off it, and comes up wherever the cursor is.
 **/
BOOST_AUTO_TEST_CASE(a_surface_reports_its_press) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    v3d::ui::Immediate::Surface seen;
    const auto panel = [&]() { seen = ui.surface("track", 40.0F); };

    // hover is found from the frame before, as every widget's is
    frame(&ui, &canvas, hover(glm::vec2(100.0F, 10.0F)), panel);
    frame(&ui, &canvas, hover(glm::vec2(100.0F, 10.0F)), panel);
    BOOST_CHECK(seen.max.x - seen.min.x > 300.0F);
    BOOST_CHECK_EQUAL(seen.max.y - seen.min.y, 40.0F);
    BOOST_CHECK(seen.hovered);
    BOOST_CHECK(!seen.held);

    frame(&ui, &canvas, press(glm::vec2(100.0F, 10.0F)), panel);
    BOOST_CHECK(seen.pressed);
    BOOST_CHECK(seen.held);

    // a drag off the surface keeps hold of it, and says where the press went down
    frame(&ui, &canvas, hold(glm::vec2(150.0F, 200.0F)), panel);
    BOOST_CHECK(!seen.pressed);
    BOOST_CHECK(!seen.hovered);
    BOOST_CHECK(seen.held);
    BOOST_CHECK(seen.cursor == glm::vec2(150.0F, 200.0F));
    BOOST_CHECK(seen.pressAt == glm::vec2(100.0F, 10.0F));

    frame(&ui, &canvas, release(glm::vec2(150.0F, 200.0F)), panel);
    BOOST_CHECK(seen.released);
    frame(&ui, &canvas, hover(glm::vec2(150.0F, 200.0F)), panel);
    BOOST_CHECK(!seen.held);
    BOOST_CHECK(!seen.released);
}

/**
 * A surface's shapes and text are drawn where they are asked for, and a shape with nothing to
 * fill draws nothing.
 **/
BOOST_AUTO_TEST_CASE(a_surface_is_drawn_into) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    std::vector<Written> written;
    v3d::ui::Immediate ui = build(&written);
    std::size_t before = 0;
    std::size_t after = 0;
    std::size_t empty = 0;
    const auto panel = [&]() {
        const v3d::ui::Immediate::Surface track = ui.surface("track", 40.0F);
        before = canvas.vertices().size();
        ui.fill(track.min, track.max, glm::vec4(1.0F));
        ui.line(track.min, track.max, 1.0F, glm::vec4(1.0F));
        ui.diamond(glm::vec2(50.0F, 20.0F), 5.0F, glm::vec4(1.0F));
        after = canvas.vertices().size();
        ui.diamond(glm::vec2(50.0F, 20.0F), 0.0F, glm::vec4(1.0F));
        empty = canvas.vertices().size();
        ui.textAt("24", glm::vec2(30.0F, 5.0F), glm::vec4(1.0F));
    };

    frame(&ui, &canvas, hover(nowhere), panel);
    BOOST_CHECK(after > before);
    BOOST_CHECK_EQUAL(empty, after);
    BOOST_REQUIRE(wrote(written, "24"));
    BOOST_CHECK_EQUAL(written.back().pen.x, 30.0F);
    BOOST_CHECK_EQUAL(ui.measure("24"), 2.0F * characterWidth);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::layer
