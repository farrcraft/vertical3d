/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/type/camera/Camera.h>
#include <demos/voxel/src/engine/CameraController.h>

#include <cstdint>
#include <vector>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boost/shared_ptr.hpp>

#include "Item.h"

/**
 * The player: a first person camera, moved by the directions held and turned by mouselook.
 *
 * The controller holds the camera's address, so a player is not copied or moved.
 **/
class Player {
 public:
    explicit Player(const glm::vec3 & pos);

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    typedef enum Movement : std::uint8_t {
        MOVE_UP = 2,
        MOVE_DOWN = 4,
        MOVE_LEFT = 8,
        MOVE_RIGHT = 16,
        MOVE_FORWARD = 32,
        MOVE_BACKWARD = 64
    } Movement;

    typedef enum Look : std::uint8_t {
        LOOK_UP = 2,
        LOOK_DOWN = 4,
        LOOK_LEFT = 8,
        LOOK_RIGHT = 16
    } Look;

    /**
     * Move by one simulation step, in whichever directions are currently held.
     * @param step seconds of simulated time
     */
    void tick(float step);

    /**
     * @return the camera the player sees through, whose view and projection are current
     **/
    const v3d::type::camera::Camera& camera() const noexcept;

    /**
     * @return what moves and turns the camera, and gives it its lens
     **/
    CameraController& controller() noexcept;
    const CameraController& controller() const noexcept;

    glm::vec3 position() const;
    void position(const glm::vec3 & pos);

    /**
     * Start or stop moving in one direction.
     * @param held whether the direction's key is held: true moves that way, false stops
     */
    void move(Movement direction, bool held);
    void look(float heading, float pitch);

    static bool checkWorldCollision(const glm::vec3 & position);

 private:
    /**
     * One direction the player can move in: the flag that holds it, the axis it moves along,
     * and the sign of the movement on that axis.
     **/
    struct Moves {
        Player::Movement move_;  // movement flag
        unsigned int axis_;  // vector position of movement
        float magnitude_;  // negative or positive movement vector
    };

    static constexpr Moves possibleMoves[] = {
        { Player::MOVE_FORWARD, 2, 1.0F },
        { Player::MOVE_BACKWARD, 2, -1.0F },
        { Player::MOVE_RIGHT, 0, 1.0F },
        { Player::MOVE_LEFT, 0, -1.0F },
        { Player::MOVE_UP, 1, 1.0F },
        { Player::MOVE_DOWN, 1, -1.0F }
    };

    /**
     * The camera's acceleration, in units per second squared, and its velocity limit, in units
     * per second. A held direction accelerates the player to the limit, and a released one slows
     * it to rest at the same rate.
     **/
    static constexpr CameraController::Motion PLAYER_MOTION{.acceleration = glm::vec3(8.0F, 8.0F, 8.0F), .limit = glm::vec3(12.0F, 12.0F, 12.0F)};

    /** How far mouselook turns the view for each pixel the mouse moves, in degrees. **/
    static constexpr float LOOK_DEGREES_PER_PIXEL = 0.1F;

    glm::vec3 position_;
    v3d::type::camera::Camera camera_;
    CameraController controller_;
    std::vector<Item> inventory_;
    unsigned int movement_;
};
