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
 * A tab strip takes a row of its own, so what a selected tab holds goes under the whole
 * strip. The selection is one of the few things kept between frames.
 **/
BOOST_AUTO_TEST_CASE(a_tab_strip_keeps_which_tab_is_selected) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    BOOST_REQUIRE(ui.tabBar("panels"));
    BOOST_CHECK(ui.tab("Turn"));
    BOOST_CHECK(!ui.tab("Entities"));
    ui.endTabBar();
    ui.end();

    // the second tab sits beside the first, not under it
    BOOST_REQUIRE_EQUAL(written.size(), 2U);
    BOOST_CHECK_CLOSE(written[0].pen.y, written[1].pen.y, 0.001F);
    BOOST_CHECK(written[1].pen.x > written[0].pen.x);

    const glm::vec2 onSecond(written[1].pen.x + 1.0F, written[1].pen.y - 2.0F);
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
    // the frame that switches still reports the tab that was selected when it began,
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
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    BOOST_REQUIRE(ui.table("units", {.columns = 2}));
    ui.column("Name", 120.0F);
    ui.column("HP", 60.0F);
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
    BOOST_CHECK(secondColumn > firstColumn + 100.0F);
    // the cells of one row share a baseline
    BOOST_CHECK_CLOSE(written[2].pen.y, written[3].pen.y, 0.001F);
    // and the next row is under both of them, back in the first column
    BOOST_CHECK_CLOSE(written[4].pen.x, firstColumn, 0.001F);
    BOOST_CHECK(written[4].pen.y > written[2].pen.y);
    BOOST_CHECK_CLOSE(written[5].pen.x, secondColumn, 0.001F);
    // what follows the table is back at the margin rather than in a column
    BOOST_CHECK_CLOSE(written[6].pen.x, firstColumn, 0.001F);
    BOOST_CHECK(written[6].pen.x < secondColumn);
}

/**
 * A table given a height scrolls its rows inside it and leaves its header above them.
 **/
BOOST_AUTO_TEST_CASE(a_table_given_a_height_keeps_its_header_above_its_rows) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 over(50.0F, 60.0F);

    const auto roster = [&ui]() {
        BOOST_REQUIRE(ui.table("units", {.columns = 2, .height = 100.0F}));
        ui.column("Name", 120.0F);
        ui.column("HP", 60.0F);
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
    ui.begin(&canvas, wheel(over, -2.0F));
    roster();
    ui.end();
    BOOST_REQUIRE_EQUAL(written.size(), 42U);
    const float header = written[0].pen.y;
    const float rowTop = written[2].pen.y;

    written.clear();
    ui.begin(&canvas, hover(over));
    roster();
    ui.end();

    BOOST_REQUIRE_EQUAL(written.size(), 42U);
    // the header has not moved and the rows have
    BOOST_CHECK_CLOSE(written[0].pen.y, header, 0.001F);
    BOOST_CHECK_CLOSE(rowTop - written[2].pen.y, ui.dressing().lineHeight * 3.0F * 2.0F, 0.001F);
}

/**
 * A table given a height is that tall whatever it holds, so what follows it does not move
 * when a row arrives.
 **/
BOOST_AUTO_TEST_CASE(a_table_given_a_height_is_that_tall_whatever_it_holds) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);

    const auto roster = [&ui](int rows) {
        BOOST_REQUIRE(ui.table("units", {.columns = 1, .height = 100.0F}));
        ui.column("Name", 0.0F);
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

    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    roster(3);
    ui.end();
    BOOST_REQUIRE(!written.empty());
    const float few = written.back().pen.y;

    written.clear();
    ui.begin(&canvas, hover(glm::vec2(-1.0F, -1.0F)));
    roster(40);
    ui.end();

    BOOST_REQUIRE(!written.empty());
    BOOST_CHECK_CLOSE(written.back().pen.y, few, 0.001F);
}

/**
 * The wheel turns the innermost region under the cursor, so a table inside a window takes
 * it from the window rather than turning both.
 **/
BOOST_AUTO_TEST_CASE(the_wheel_turns_a_table_rather_than_the_window_holding_it) {
    std::vector<Written> written;
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(400, 300));
    v3d::ui::Immediate ui = build(&written);
    const glm::vec2 corner(20.0F, 20.0F);
    const glm::vec2 size(220.0F, 160.0F);
    const glm::vec2 overTable(60.0F, 100.0F);

    const auto panel = [&ui, &corner, &size]() {
        BOOST_REQUIRE(ui.window("Roster", {.position = corner, .size = size}, 1.0F));
        ui.text("above");
        BOOST_REQUIRE(ui.table("units", {.columns = 1, .height = 80.0F}));
        ui.column("Name", 0.0F);
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
    ui.begin(&canvas, wheel(overTable, -2.0F));
    panel();
    ui.end();
    BOOST_REQUIRE(written.size() > 3U);
    const float above = written[1].pen.y;
    const float header = written[2].pen.y;
    const float rowTop = written[3].pen.y;

    written.clear();
    ui.begin(&canvas, hover(overTable));
    panel();
    ui.end();

    BOOST_REQUIRE(written.size() > 3U);
    // the window kept its place, so what is above the table and the header are where they
    // were, and only the rows moved
    BOOST_CHECK_CLOSE(written[1].pen.y, above, 0.001F);
    BOOST_CHECK_CLOSE(written[2].pen.y, header, 0.001F);
    BOOST_CHECK(written[3].pen.y < rowTop);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::layer
