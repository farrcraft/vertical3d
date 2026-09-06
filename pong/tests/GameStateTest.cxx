/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../src/GameState.h"

/**
 * The values a round starts on. The ball size is what the collision tests measure against,
 * and the max score is what ends a game. The start speed is pixels per second, because the
 * scene advances by a fixed step rather than by a frame.
 **/
BOOST_AUTO_TEST_CASE(game_state_defaults_test) {
    GameState state;

    BOOST_TEST(state.ballSize() == 10.0f);
    BOOST_TEST(state.ballSpeedup() == 1.0f);
    BOOST_TEST(state.ballStartSpeed() == 60.0f);
    BOOST_TEST(state.maxScore() == 5);
    BOOST_TEST(state.coop());
    BOOST_TEST(!state.paused());
}

/**
 * reset() puts the ball speeds back and unpauses, but leaves the options a player chose -
 * the mode and the target score outlive a round.
 **/
BOOST_AUTO_TEST_CASE(game_state_reset_test) {
    GameState state;

    state.coop(false);
    state.maxScore(11);
    state.ballStartSpeed(4.0f);
    state.pause(true);

    state.reset();

    BOOST_TEST(state.ballStartSpeed() == 60.0f);
    BOOST_TEST(state.ballSpeedup() == 1.0f);
    BOOST_TEST(!state.paused());
    BOOST_TEST(!state.coop());
    BOOST_TEST(state.maxScore() == 11);
}

BOOST_AUTO_TEST_CASE(game_state_pause_test) {
    GameState state;

    state.pause(true);
    BOOST_TEST(state.paused());

    state.pause(false);
    BOOST_TEST(!state.paused());
}
