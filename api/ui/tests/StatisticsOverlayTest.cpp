/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Canvas.h>
#include <api/ui/StatisticsOverlay.h>

#include <array>
#include <cstdint>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

const std::uint64_t millisecond = 1000000;

};  // namespace

BOOST_AUTO_TEST_SUITE(statistics_overlay_test)

/**
 * A frame time reads as milliseconds to one decimal, and the rate it implies sits beside it.
 **/
BOOST_AUTO_TEST_CASE(a_frame_reads_as_milliseconds_and_a_rate) {
    v3d::ui::StatisticsOverlay::Sample sample;
    sample.mean = 16 * millisecond + 667000;
    sample.last = 17 * millisecond;
    sample.steps = 1;

    const std::array<std::string, v3d::ui::StatisticsOverlay::rows> lines =
        v3d::ui::StatisticsOverlay::lines(sample);

    BOOST_CHECK_EQUAL(lines[0], "16.7 ms  60 fps");
    BOOST_CHECK_EQUAL(lines[1], "last 17.0 ms");
    BOOST_CHECK_EQUAL(lines[2], "steps 1");
}

/**
 * Before the first frame there is no duration to divide into, so the rate is left off
 * rather than dividing by zero.
 **/
BOOST_AUTO_TEST_CASE(no_rate_is_shown_before_the_first_frame) {
    const std::array<std::string, v3d::ui::StatisticsOverlay::rows> lines =
        v3d::ui::StatisticsOverlay::lines(v3d::ui::StatisticsOverlay::Sample());

    BOOST_CHECK_EQUAL(lines[0], "0.0 ms");
    BOOST_CHECK_EQUAL(lines[1], "last 0.0 ms");
    BOOST_CHECK_EQUAL(lines[2], "steps 0");
}

/**
 * The steps line is what says the accumulator's clamp is doing real work, so a frame that
 * owed several of them has to be able to say so.
 **/
BOOST_AUTO_TEST_CASE(a_frame_that_owed_several_steps_says_so) {
    v3d::ui::StatisticsOverlay::Sample sample;
    sample.mean = 100 * millisecond;
    sample.last = 250 * millisecond;
    sample.steps = 6;

    const std::array<std::string, v3d::ui::StatisticsOverlay::rows> lines =
        v3d::ui::StatisticsOverlay::lines(sample);

    BOOST_CHECK_EQUAL(lines[0], "100.0 ms  10 fps");
    BOOST_CHECK_EQUAL(lines[1], "last 250.0 ms");
    BOOST_CHECK_EQUAL(lines[2], "steps 6");
}

/**
 * An overlay starts hidden and toggles both ways.
 **/
BOOST_AUTO_TEST_CASE(an_overlay_starts_hidden_and_toggles_both_ways) {
    const boost::shared_ptr<v3d::ui::TextRenderer> text;
    v3d::ui::StatisticsOverlay overlay(text);

    BOOST_CHECK(!overlay.visible());
    overlay.toggle();
    BOOST_CHECK(overlay.visible());
    overlay.toggle();
    BOOST_CHECK(!overlay.visible());

    overlay.visible(true);
    BOOST_CHECK(overlay.visible());
}

/**
 * Nothing is drawn without a text renderer to draw it with, which is what a font that
 * would not load leaves the overlay holding.
 **/
BOOST_AUTO_TEST_CASE(nothing_is_drawn_without_a_text_renderer) {
    const boost::shared_ptr<v3d::ui::TextRenderer> text;
    v3d::ui::StatisticsOverlay overlay(text);
    overlay.visible(true);

    v3d::render::realtime::Canvas canvas;
    canvas.resize(640, 480);
    overlay.draw(&canvas, v3d::ui::StatisticsOverlay::Sample());

    BOOST_CHECK(canvas.empty());
}

BOOST_AUTO_TEST_SUITE_END()
