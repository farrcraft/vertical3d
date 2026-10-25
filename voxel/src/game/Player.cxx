/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Player.h"

#include <voxel/src/engine/Camera.h>

#include <iterator>

namespace {

typedef struct Moves {
    Player::Movement move_;  // movement flag
    unsigned int axis_;  // vector position of movement
    float magnitude_;  // negative or positive movement vector
} Moves;

Moves possibleMoves[] = {
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
constexpr glm::vec3 PLAYER_ACCELERATION(8.0F, 8.0F, 8.0F);
constexpr glm::vec3 PLAYER_VELOCITY(12.0F, 12.0F, 12.0F);

/** How far mouselook turns the view for each pixel the mouse moves, in degrees. **/
constexpr float LOOK_DEGREES_PER_PIXEL = 0.1F;

};  // namespace

Player::Player(const glm::vec3 & pos) :
    position_(pos),
    movement_(0) {
    camera_.reset(new Camera());
    camera_->behavior(Camera::CAMERA_BEHAVIOR_FIRST_PERSON);
    camera_->position(position_);
    camera_->acceleration(PLAYER_ACCELERATION);
    camera_->velocity(PLAYER_VELOCITY);
}

glm::vec3 Player::position() const {
    return position_;
}

boost::shared_ptr<Camera> Player::camera() {
    return camera_;
}

void Player::position(const glm::vec3 & pos) {
    position_ = pos;
}

void Player::move(Movement direction, bool held) {
    // the speed follows in tick(), which accelerates a held direction and slows a released one
    if (held) {
        movement_ |= direction;
    } else {
        movement_ &= ~direction;
    }
}

void Player::look(float heading, float pitch) {
    camera_->rotate(heading * LOOK_DEGREES_PER_PIXEL, pitch * LOOK_DEGREES_PER_PIXEL, 0.0F);
}

void Player::tick(float step) {
    // update player position based on current movement flags
    glm::vec3 dir(0.0F, 0.0F, 0.0F);
    for (unsigned int i = 0; i < std::size(possibleMoves); i++) {
        if ((movement_ & possibleMoves[i].move_) != 0) {
            dir[static_cast<glm::length_t>(possibleMoves[i].axis_)] = possibleMoves[i].magnitude_;
        }
    }
    const glm::vec3 displacement = camera_->calculateDisplacement(dir, step);
    const glm::vec3 position = camera_->calculateMovement(displacement);
    if (!checkWorldCollision(position)) {
        camera_->position(position);
        position_ = position;
    }
    camera_->updateVelocity(dir, step);
}

bool Player::checkWorldCollision(const glm::vec3 & /* position */) {
    /*
    convert position into voxel coordinate space
    convert voxel position into chunk coordinate space
    get containing chunk
    if voxel at position is active
        return true
    */
    return false;
}

