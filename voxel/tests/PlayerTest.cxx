/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <voxel/src/game/Player.h>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

/**
 * A direction moves the player while it is held and stops when it is not. Saying a direction
 * is held twice keeps it held. So a key released while the menu was up stops the player once
 * the menu closes, rather than leaving the player moving until the key is pressed again.
 **/
BOOST_AUTO_TEST_CASE(player_moves_while_a_direction_is_held_test) {
    Player player(glm::vec3(0.0f));

    player.move(Player::MOVE_FORWARD, true);
    player.move(Player::MOVE_FORWARD, true);
    player.tick(0.1f);
    const glm::vec3 moved = player.position();
    BOOST_TEST(moved.z != 0.0f);

    player.move(Player::MOVE_FORWARD, false);
    player.move(Player::MOVE_FORWARD, false);
    player.tick(0.1f);
    BOOST_TEST(player.position().z == moved.z);
}
