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
    glm::vec3 position_;
    v3d::type::camera::Camera camera_;
    CameraController controller_;
    std::vector<Item> inventory_;
    unsigned int movement_;
};
