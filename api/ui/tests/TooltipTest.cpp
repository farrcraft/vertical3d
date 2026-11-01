/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Component.h>
#include <api/ui/component/Button.h>
#include <api/ui/shell/Tooltip.h>

#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

const v3d::ui::shell::Tooltip::Delays delays{.first = 0.5F, .following = 0.1F};

boost::shared_ptr<v3d::ui::Component> tipped(const std::string& text) {
    const boost::shared_ptr<v3d::ui::component::Button> button = boost::make_shared<v3d::ui::component::Button>();
    button->tooltip(text);
    return button;
}

BOOST_AUTO_TEST_SUITE(tooltip_test)

/**
 * A tooltip is due once the cursor has rested on its component for the delay.
 **/
BOOST_AUTO_TEST_CASE(a_tooltip_waits_for_the_cursor_to_rest) {
    v3d::ui::shell::Tooltip tooltip;
    const boost::shared_ptr<v3d::ui::Component> move = tipped("Move");
    BOOST_TEST(!tooltip.due(move, 1.0F, delays));
    BOOST_TEST(!tooltip.due(move, 1.4F, delays));
    BOOST_TEST(tooltip.due(move, 1.5F, delays));
}

/**
 * Moving on from a tooltip that was showing shows the next one after the shorter delay, and a
 * cursor that was off every tooltip for longer than the full delay waits it again.
 **/
BOOST_AUTO_TEST_CASE(moving_on_from_a_tooltip_is_quicker) {
    v3d::ui::shell::Tooltip tooltip;
    const boost::shared_ptr<v3d::ui::Component> move = tipped("Move");
    const boost::shared_ptr<v3d::ui::Component> turn = tipped("Rotate");
    tooltip.due(move, 0.0F, delays);
    BOOST_REQUIRE(tooltip.due(move, 0.6F, delays));

    BOOST_TEST(!tooltip.due(turn, 0.65F, delays));
    BOOST_TEST(tooltip.due(turn, 0.75F, delays));

    tooltip.due(nullptr, 0.8F, delays);
    tooltip.due(move, 2.0F, delays);
    BOOST_TEST(!tooltip.due(move, 2.2F, delays));
    BOOST_TEST(tooltip.due(move, 2.5F, delays));
}

/**
 * A component with no tooltip, and nothing at all, never shows one.
 **/
BOOST_AUTO_TEST_CASE(nothing_to_say_shows_nothing) {
    v3d::ui::shell::Tooltip tooltip;
    const boost::shared_ptr<v3d::ui::Component> plain = tipped("");
    tooltip.due(plain, 0.0F, delays);
    BOOST_TEST(!tooltip.due(plain, 5.0F, delays));
    BOOST_TEST(!tooltip.due(nullptr, 9.0F, delays));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
