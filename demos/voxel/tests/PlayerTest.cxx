/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <demos/voxel/src/engine/CameraController.h>
#include <demos/voxel/src/game/Player.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

namespace {

/**
 * A direction moves the player while it is held. The first step from rest only gathers speed,
 * so the player has moved after the second. Saying a direction is held twice keeps it held.
 * Once it is released the player slows to a stop rather than stopping at once, and a key
 * released while the menu was up therefore brings the player to rest once the menu closes.
 **/
BOOST_AUTO_TEST_CASE(player_moves_while_a_direction_is_held_test) {
    Player player(glm::vec3(0.0F));

    player.move(Player::MOVE_FORWARD, true);
    player.move(Player::MOVE_FORWARD, true);
    player.tick(0.1F);
    player.tick(0.1F);
    const glm::vec3 moved = player.position();
    BOOST_TEST(moved.z != 0.0F);

    player.move(Player::MOVE_FORWARD, false);
    player.move(Player::MOVE_FORWARD, false);
    // still moving, more slowly than it was
    player.tick(0.1F);
    const glm::vec3 gliding = player.position();
    BOOST_TEST(gliding.z != moved.z);

    // and at rest within the time the deceleration takes to cancel the speed reached
    for (int step = 0; step < 20; step++) {
        player.tick(0.1F);
    }
    const glm::vec3 rested = player.position();
    player.tick(0.1F);
    BOOST_TEST(player.position().z == rested.z);
}

/**
 * A held direction accelerates the player to its speed limit, 12 units a second, and no
 * further.
 **/
BOOST_AUTO_TEST_CASE(player_reaches_its_speed_limit_test) {
    Player player(glm::vec3(0.0F));
    player.move(Player::MOVE_FORWARD, true);
    for (int step = 0; step < 30; step++) {
        player.tick(0.1F);
    }
    BOOST_CHECK_CLOSE(std::abs(player.controller().velocity().z), 12.0F, 0.01F);
}

/**
 * Mouselook turns the view by a tenth of a degree for each pixel the mouse moves, and does not
 * roll it. A hundred pixels to the right is ten degrees right.
 **/
BOOST_AUTO_TEST_CASE(player_look_turns_by_degrees_per_pixel_test) {
    Player player(glm::vec3(0.0F));
    player.look(100.0F, 0.0F);
    const glm::vec3 direction = player.camera().profile().direction();
    const float angle = std::atan2(direction.x, -direction.z);
    BOOST_CHECK_CLOSE(angle, glm::radians(10.0F), 0.1F);
    BOOST_CHECK_SMALL(glm::length(player.camera().profile().up() - glm::vec3(0.0F, 1.0F, 0.0F)), 0.0001F);
}

};  // namespace
