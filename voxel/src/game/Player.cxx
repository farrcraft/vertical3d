/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Player.h"
#include "../engine/Camera.h"

typedef struct Moves {
    unsigned int move_;  // movement flag
    unsigned int axis_;  // vector position of movement
    float magnitude_;  // negative or positive movement vector
} Moves;

Moves possibleMoves[] = {
    { Player::MOVE_FORWARD, 2, 1.0f },
    { Player::MOVE_BACKWARD, 2, -1.0f },
    { Player::MOVE_RIGHT, 0, 1.0f },
    { Player::MOVE_LEFT, 0, -1.0f },
    { Player::MOVE_UP, 1, 1.0f },
    { Player::MOVE_DOWN, 1, -1.0f }
};

Player::Player(const glm::vec3 & pos) :
    position_(pos),
    movement_(0) {
    camera_.reset(new Camera());
    camera_->behavior(Camera::CAMERA_BEHAVIOR_FIRST_PERSON);
    camera_->position(position_);
    camera_->acceleration(glm::vec3(8.0f, 8.0f, 8.0f));
    camera_->velocity(glm::vec3(2.0f, 2.0f, 2.0f));
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

void Player::move(Movement direction) {
    for (unsigned int i = 0; i < 6; i++) {
        if (direction == possibleMoves[i].move_) {
            glm::vec3 velocity = camera_->currentVelocity();
            if (movement_ & direction) {
                velocity[possibleMoves[i].axis_] = 0.0f;
                movement_ &= ~direction;
            } else {
                velocity[possibleMoves[i].axis_] = possibleMoves[i].magnitude_;
                movement_ |= direction;
            }
            camera_->currentVelocity(velocity.x, velocity.y, velocity.z);
            return;
        }
    }
}

void Player::look(float heading, float pitch) {
    const float cameraRotationSpeed = 0.01f;

    heading *= cameraRotationSpeed;
    pitch *= cameraRotationSpeed;

    camera_->rotate(heading, pitch, 0.0f);
}

void Player::tick(unsigned int delta) {
    // update player position based on current movement flags
    glm::vec3 dir(0.0f, 0.0f, 0.0f);
    for (unsigned int i = 0; i < 6; i++) {
        if (movement_ & possibleMoves[i].move_) {
            dir[possibleMoves[i].axis_] = possibleMoves[i].magnitude_;
        }
    }
    float elapsedTimeSec = 0.1f;
    // camera_->updatePosition(dir, elapsedTimeSec);
    glm::vec3 displacement = camera_->calculateDisplacement(dir, elapsedTimeSec);
    glm::vec3 position = camera_->calculateMovement(displacement);
    if (!checkWorldCollision(position)) {
        camera_->position(position);

        // position_ = camera_->position();
        position_ = position;
    }
}

bool Player::checkWorldCollision(const glm::vec3 & position) {
    /*
    convert position into voxel coordinate space
    convert voxel position into chunk coordinate space
    get containing chunk
    if voxel at position is active
        return true
    */
    glm::ivec3 voxelPosition;

    return false;
}

