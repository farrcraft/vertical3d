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
};  // namespace v3d::ui::layer
