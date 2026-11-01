/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>

#include <algorithm>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Layer.h"

namespace v3d::ui::layer {
namespace {

BOOST_AUTO_TEST_SUITE(immediate_test)

/**
 * A region holds its contents inside itself with no title bar, and clips them to its edges.
 **/
BOOST_AUTO_TEST_CASE(a_region_holds_its_contents_with_no_title) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(50.0F, 40.0F);
    const glm::vec2 size(200.0F, 150.0F);

    ui.begin(&canvas, hover(nowhere));
    ui.region("Outliner", {.position = corner, .size = size}, 1.0F);
    ui.button("Cube");
    ui.endRegion();
    ui.end();

    // the name is an id, not a title, so the line is all that is written, at the top
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_EQUAL(written[0].text, "Cube");
    BOOST_CHECK(written[0].pen.x > corner.x);
    BOOST_CHECK(written[0].pen.y > corner.y);
    BOOST_CHECK(written[0].pen.y < corner.y + ui.dressing().barHeight);
    const v3d::render::realtime::canvas::Canvas::Batch& cut = canvas.batches().back();
    BOOST_REQUIRE(cut.clipped);
    BOOST_CHECK_CLOSE(cut.clip.y, corner.y, 0.001F);
    BOOST_CHECK_CLOSE(cut.clip.w, corner.y + size.y - ui.dressing().borderWidth, 0.001F);
}

/**
 * A region takes the cursor from a window drawn before it, as a window drawn later would.
 **/
BOOST_AUTO_TEST_CASE(a_region_takes_the_cursor_from_what_is_under_it) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    const glm::vec2 shared(60.0F, 75.0F);
    bool beneath = false;
    const auto panel = [&]() {
        ui.window("under", {.position = glm::vec2(40.0F, 40.0F), .size = glm::vec2(200.0F, 200.0F)}, 1.0F);
        beneath = ui.button("beneath");
        ui.endWindow();
        ui.region("over", {.position = glm::vec2(50.0F, 50.0F), .size = glm::vec2(200.0F, 200.0F)}, 0.0F);
        ui.endRegion();
    };

    click(&ui, &canvas, shared, panel);
    BOOST_CHECK(!beneath);
    BOOST_CHECK(ui.capturing());
}

/**
 * The wheel over a region with more in it than it shows scrolls it.
 **/
BOOST_AUTO_TEST_CASE(a_region_scrolls_under_the_wheel) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 inside(60.0F, 60.0F);
    const auto panel = [&]() {
        ui.region("list", {.position = glm::vec2(50.0F, 50.0F), .size = glm::vec2(200.0F, 60.0F)}, 1.0F);
        for (int row = 0; row < 20; row++) {
            ui.text("row " + std::to_string(row));
        }
        ui.endRegion();
    };

    frame(&ui, &canvas, hover(inside), panel);
    written.clear();
    frame(&ui, &canvas, hover(inside), panel);
    const float top = written.front().pen.y;
    written.clear();
    // the wheel scrolls once the frame it turned on has measured the rows
    frame(&ui, &canvas, wheel(inside, -1.0F), panel);
    written.clear();
    frame(&ui, &canvas, hover(inside), panel);
    BOOST_CHECK(written.front().pen.y < top);
}

/**
 * A tree row is indented a line for each level, and one with something beneath it puts its
 * mark in that place.
 **/
BOOST_AUTO_TEST_CASE(a_tree_row_is_indented_by_its_depth) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    bool open = true;

    ui.begin(&canvas, hover(nowhere));
    ui.treeRow("Rig", {.depth = 0, .open = &open});
    ui.treeRow("Arm", {.depth = 2});
    ui.end();

    const float line = ui.dressing().lineHeight;
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_CLOSE(written[0].pen.x, line, 0.001F);
    BOOST_CHECK_CLOSE(written[1].pen.x, line * 3.0F, 0.001F);
}

/**
 * A click on a row's mark folds or opens it and is not a click on the row. A click anywhere
 * else on the row is.
 **/
BOOST_AUTO_TEST_CASE(a_tree_rows_mark_folds_it) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    bool open = true;
    bool clicked = false;
    const auto panel = [&]() { clicked = ui.treeRow("Rig", {.depth = 1, .open = &open, .selected = false}); };
    const float line = ui.dressing().lineHeight;
    const glm::vec2 onMark(line * 1.5F, line * 0.5F);
    const glm::vec2 onLabel(line * 4.0F, line * 0.5F);

    click(&ui, &canvas, onMark, panel);
    BOOST_CHECK(!open);
    BOOST_CHECK(!clicked);
    click(&ui, &canvas, onMark, panel);
    BOOST_CHECK(open);

    click(&ui, &canvas, onLabel, panel);
    BOOST_CHECK(clicked);
    BOOST_CHECK(open);
}

/**
 * A chosen row writes its label in the active text colour.
 **/
BOOST_AUTO_TEST_CASE(a_chosen_tree_row_is_highlighted) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(nowhere));
    ui.treeRow("Cube", {.selected = true});
    ui.treeRow("Cone", {});
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK(written[0].colour == ui.dressing().activeText);
    BOOST_CHECK(written[1].colour == ui.dressing().text);
}

/**
 * A row with an icon draws it after the mark's place, tinted, and moves its label past it.
 **/
BOOST_AUTO_TEST_CASE(a_tree_row_leads_with_its_icon) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec4 tint(0.9F, 0.6F, 0.2F, 1.0F);
    const v3d::ui::Image icon(v3d::render::realtime::TextureHandle(5));

    ui.begin(&canvas, hover(nowhere));
    ui.treeRow("Cube", {.depth = 0, .open = nullptr, .selected = false, .icon = icon, .iconTint = tint});
    ui.end();

    const float line = ui.dressing().lineHeight;
    const float side = std::min(ui.dressing().iconSize, line);
    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK_CLOSE(written[0].pen.x, line + side + (ui.dressing().padding * 0.5F), 0.001F);
    const auto drawn = std::ranges::find_if(canvas.vertices(), [&tint](const auto& vertex) { return vertex.colour == tint; });
    BOOST_REQUIRE(drawn != canvas.vertices().end());
    BOOST_CHECK_CLOSE(drawn->position.x, line, 0.001F);
}

/**
 * Text can be written in a colour of its own, and an image is drawn the size given, tinted,
 * and centred down a line when it is shorter than one.
 **/
BOOST_AUTO_TEST_CASE(text_in_a_colour_and_an_image_at_the_pen) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec4 red(0.85F, 0.35F, 0.37F, 1.0F);
    const glm::vec4 tint(0.2F, 0.7F, 0.3F, 1.0F);

    ui.begin(&canvas, hover(nowhere));
    ui.image(v3d::ui::Image(v3d::render::realtime::TextureHandle(5)), glm::vec2(10.0F, 10.0F), tint);
    ui.text("X", red);
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 1U);
    BOOST_CHECK(written[0].colour == red);
    const auto drawn = std::ranges::find_if(canvas.vertices(), [&tint](const auto& vertex) { return vertex.colour == tint; });
    BOOST_REQUIRE(drawn != canvas.vertices().end());
    // the first item is at the origin, and the image is centred down its line
    BOOST_CHECK_CLOSE(drawn->position.x, 0.0F, 0.001F);
    BOOST_CHECK_CLOSE(drawn->position.y, (ui.dressing().lineHeight - 10.0F) * 0.5F, 0.001F);
}

/**
 * Dragging across a number moves it by the speed for each pixel, once the press has travelled
 * a few pixels, and letting go finishes the edit.
 **/
BOOST_AUTO_TEST_CASE(a_dragged_number_moves_by_its_speed) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    float value = 1.0F;
    bool finished = false;
    const auto panel = [&]() {
        ui.dragFloat("x", &value, 0.5F);
        finished = ui.itemFinished();
    };
    const glm::vec2 start(100.0F, 10.0F);

    frame(&ui, &canvas, hover(start), panel);
    frame(&ui, &canvas, press(start), panel);
    // within the threshold nothing moves
    frame(&ui, &canvas, hold(start + glm::vec2(2.0F, 0.0F)), panel);
    BOOST_CHECK_EQUAL(value, 1.0F);
    frame(&ui, &canvas, hold(start + glm::vec2(12.0F, 0.0F)), panel);
    BOOST_CHECK_CLOSE(value, 6.0F, 0.001F);
    BOOST_CHECK(!finished);
    frame(&ui, &canvas, release(start + glm::vec2(12.0F, 0.0F)), panel);
    BOOST_CHECK(finished);
    BOOST_CHECK(!ui.typing());
    frame(&ui, &canvas, hover(start), panel);
    BOOST_CHECK(!finished);
}

/**
 * A click that does not drag types into the number. A return takes what was typed and
 * finishes the edit.
 **/
BOOST_AUTO_TEST_CASE(a_clicked_number_is_typed_into) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    float value = 1.0F;
    bool changed = false;
    bool finished = false;
    const auto panel = [&]() {
        changed = ui.dragFloat("x", &value, 0.5F);
        finished = ui.itemFinished();
    };

    click(&ui, &canvas, firstRow, panel);
    BOOST_REQUIRE(ui.typing());
    // the whole value is chosen, so typing replaces it
    BOOST_CHECK(ui.typed("-2.5"));
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK_EQUAL(value, 1.0F);
    BOOST_CHECK(!changed);

    BOOST_CHECK(ui.press("return"));
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK_EQUAL(value, -2.5F);
    BOOST_CHECK(changed);
    BOOST_CHECK(finished);
    BOOST_CHECK(!ui.typing());
}

/**
 * Escape, and text that is not a number, leave the value as it was, and still finish the edit.
 **/
BOOST_AUTO_TEST_CASE(a_number_keeps_its_value_on_escape_or_nonsense) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    float value = 1.0F;
    bool finished = false;
    const auto panel = [&]() {
        ui.dragFloat("x", &value, 0.5F);
        finished = ui.itemFinished();
    };

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(ui.typed("7"));
    BOOST_CHECK(ui.press("escape"));
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK_EQUAL(value, 1.0F);
    BOOST_CHECK(finished);

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(ui.typed("seven"));
    BOOST_CHECK(ui.press("return"));
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK_EQUAL(value, 1.0F);
    BOOST_CHECK(finished);
}

/**
 * A text field finishes its edit when it gives the keyboard up.
 **/
BOOST_AUTO_TEST_CASE(a_text_field_finishes_when_it_lets_go) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build();
    std::string name = "Cube";
    bool finished = false;
    const auto panel = [&]() {
        ui.textField("Name", &name);
        finished = ui.itemFinished();
    };

    click(&ui, &canvas, firstRow, panel);
    BOOST_CHECK(!finished);
    BOOST_CHECK(ui.typed("s"));
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK(!finished);
    BOOST_CHECK(ui.press("return"));
    frame(&ui, &canvas, hover(firstRow), panel);
    BOOST_CHECK(finished);
    BOOST_CHECK_EQUAL(name, "Cubes");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::layer
