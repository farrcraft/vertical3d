/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <pong/src/GameState.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * The values a round starts on. The collision tests measure against the ball size, and
 * reaching the max score ends a game. The start speed is in court units per second.
 **/
BOOST_AUTO_TEST_CASE(game_state_defaults_test) {
    const GameState state;

    BOOST_TEST(state.ballSize() == 10.0F);
    BOOST_TEST(state.ballStartSpeed() == 60.0F);
    BOOST_TEST(state.maxScore() == 5);
    BOOST_TEST(state.coop());
    BOOST_TEST(!state.paused());
}

/**
 * reset() restores the ball's start speed. It keeps the mode and the target score a player
 * chose, and leaves the pause to the menu that set it.
 **/
BOOST_AUTO_TEST_CASE(game_state_reset_test) {
    GameState state;

    state.coop(false);
    state.maxScore(11);
    state.ballStartSpeed(4.0F);
    state.pause(true);

    state.reset();

    BOOST_TEST(state.ballStartSpeed() == 60.0F);
    BOOST_TEST(state.paused());
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

};  // namespace
