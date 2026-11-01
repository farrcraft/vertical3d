/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Transition.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/vec4.hpp>

namespace {

constexpr glm::vec4 red(1.0F, 0.0F, 0.0F, 1.0F);
constexpr glm::vec4 blue(0.0F, 0.0F, 1.0F, 1.0F);

/**
 * @return whether two colours agree in every channel, to a tolerance
 **/
bool near(const glm::vec4& one, const glm::vec4& other) {
    const glm::vec4 gap = one - other;
    return std::abs(gap.r) < 0.001F && std::abs(gap.g) < 0.001F && std::abs(gap.b) < 0.001F && std::abs(gap.a) < 0.001F;
}

BOOST_AUTO_TEST_SUITE(transition_test)

/**
 * The first colour asked for is shown at once, so nothing eases in from black when it is first
 * drawn.
 **/
BOOST_AUTO_TEST_CASE(the_first_colour_is_shown_at_once) {
    v3d::ui::Transition transition;
    BOOST_CHECK(near(transition.toward(red, {.now = 5.0F}, 0.1F), red));
}

/**
 * A change eases in a straight line over its duration, and holds the new colour after it.
 **/
BOOST_AUTO_TEST_CASE(a_change_eases_over_its_duration) {
    v3d::ui::Transition transition;
    transition.toward(red, {.now = 0.0F}, 0.1F);

    BOOST_CHECK(near(transition.toward(blue, {.now = 1.0F}, 0.1F), red));
    BOOST_CHECK(near(transition.toward(blue, {.now = 1.05F}, 0.1F), glm::vec4(0.5F, 0.0F, 0.5F, 1.0F)));
    BOOST_CHECK(near(transition.toward(blue, {.now = 1.1F}, 0.1F), blue));
    BOOST_CHECK(near(transition.toward(blue, {.now = 9.0F}, 0.1F), blue));
}

/**
 * A change made partway through another starts from where that one had reached, so a colour
 * never jumps.
 **/
BOOST_AUTO_TEST_CASE(a_change_partway_eases_on_from_where_it_reached) {
    v3d::ui::Transition transition;
    transition.toward(red, {.now = 0.0F}, 0.1F);
    transition.toward(blue, {.now = 1.0F}, 0.1F);

    // halfway to blue, it is asked for red again
    const glm::vec4 halfway(0.5F, 0.0F, 0.5F, 1.0F);
    BOOST_CHECK(near(transition.toward(red, {.now = 1.05F}, 0.1F), halfway));
    BOOST_CHECK(near(transition.toward(red, {.now = 1.1F}, 0.1F), glm::vec4(0.75F, 0.0F, 0.25F, 1.0F)));
    BOOST_CHECK(near(transition.toward(red, {.now = 1.15F}, 0.1F), red));
}

/**
 * A duration of zero turns the transition off: a change shows at once.
 **/
BOOST_AUTO_TEST_CASE(no_duration_changes_at_once) {
    v3d::ui::Transition transition;
    transition.toward(red, {.now = 0.0F}, 0.0F);
    BOOST_CHECK(near(transition.toward(blue, {.now = 1.0F}, 0.0F), blue));
}

/**
 * Easing away fades the last colour out in its own colour, rather than through another.
 **/
BOOST_AUTO_TEST_CASE(easing_away_fades_the_colour_out) {
    v3d::ui::Transition transition;
    transition.toward(red, {.now = 0.0F}, 0.1F);

    BOOST_CHECK(near(transition.away({.now = 1.05F}, 0.1F), red));
    BOOST_CHECK(near(transition.away({.now = 1.1F}, 0.1F), glm::vec4(1.0F, 0.0F, 0.0F, 0.5F)));
    BOOST_CHECK(near(transition.away({.now = 1.15F}, 0.1F), glm::vec4(1.0F, 0.0F, 0.0F, 0.0F)));
}

/**
 * A colour easing in from nothing shown eases in its own colour, rather than from the black of
 * an empty fill.
 **/
BOOST_AUTO_TEST_CASE(easing_in_from_nothing_keeps_the_colour) {
    v3d::ui::Transition transition;
    transition.away({.now = 0.0F}, 0.1F);

    transition.toward(red, {.now = 1.0F}, 0.1F);
    BOOST_CHECK(near(transition.toward(red, {.now = 1.05F}, 0.1F), glm::vec4(1.0F, 0.0F, 0.0F, 0.5F)));
}

/**
 * Something never shown and asked to ease away is not drawn at all.
 **/
BOOST_AUTO_TEST_CASE(easing_away_from_nothing_shows_nothing) {
    v3d::ui::Transition transition;
    BOOST_CHECK_SMALL(transition.away({.now = 1.0F}, 0.1F).a, 0.001F);
}

/**
 * Opening rises from 0 on the frame something is first drawn to 1 over its duration, while it
 * is drawn on every frame.
 **/
BOOST_AUTO_TEST_CASE(opening_rises_while_drawn_on_every_frame) {
    v3d::ui::Opening opening;
    BOOST_CHECK_SMALL(opening.progress({.now = 2.0F, .frame = 10}, 0.2F), 0.001F);
    BOOST_CHECK_CLOSE(opening.progress({.now = 2.1F, .frame = 11}, 0.2F), 0.5F, 0.1F);
    // drawn twice in one frame is still the same opening
    BOOST_CHECK_CLOSE(opening.progress({.now = 2.1F, .frame = 11}, 0.2F), 0.5F, 0.1F);
    BOOST_CHECK_CLOSE(opening.progress({.now = 2.5F, .frame = 12}, 0.2F), 1.0F, 0.001F);
}

/**
 * Something not drawn on the frame before has been closed between, and opens again.
 **/
BOOST_AUTO_TEST_CASE(a_skipped_frame_opens_again) {
    v3d::ui::Opening opening;
    opening.progress({.now = 0.0F, .frame = 1}, 0.2F);
    BOOST_CHECK_CLOSE(opening.progress({.now = 1.0F, .frame = 2}, 0.2F), 1.0F, 0.001F);

    BOOST_CHECK_SMALL(opening.progress({.now = 3.0F, .frame = 9}, 0.2F), 0.001F);
}

/**
 * A duration of zero is open at once.
 **/
BOOST_AUTO_TEST_CASE(no_duration_is_open_at_once) {
    v3d::ui::Opening opening;
    BOOST_CHECK_CLOSE(opening.progress({.now = 0.0F, .frame = 1}, 0.0F), 1.0F, 0.001F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
