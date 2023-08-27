/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongScene.h"

#include <iostream>
#include <string>

#include "../../api/event/Sound.h"

PongScene::PongScene(entt::registry& registry, const boost::shared_ptr<entt::dispatcher>& dispatcher) : 
    dispatcher_(dispatcher),
    registry_(registry),
    left_(registry), 
    right_(registry),
    ball_(registry) {
    left_.color(glm::vec3(1.0f, 1.0f, 1.0f));
    right_.color(glm::vec3(1.0f, 1.0f, 1.0f));
}

PongScene::~PongScene() {
}

void PongScene::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void PongScene::tick() {
    if (gameState_.paused()) {
        return;
    }

    // check for victory conditions
    if (left_.score() == gameState_.maxScore() ||
        right_.score() == gameState_.maxScore()) {
        reset();
        dispatcher_->trigger(v3d::event::Sound("victory"));
    }
    glm::vec2 ball_pos = ball_.position();
    glm::vec2 ball_dir = ball_.direction();

    // give AI a turn in single player mode
    if (!gameState_.coop()) {
        // is the ball headed towards the ai's paddle (towards the right side)?
        if (ball_dir[0] > 0.0f) {
            if (ball_pos[1] < right_.position()) {
                right_.down(true);
                right_.up(false);
            } else if (ball_pos[1] > right_.position()) {
                right_.down(false);
                right_.up(true);
            }
        } else {
            // no need to move the paddle if the ball is moving away from it
            right_.up(false);
            right_.down(false);
        }
    }

    // assume both paddles are the same dimensions
    float paddle_mid = left_.length() / 2.0f;
    float paddle_size = left_.size();

    // do paddle/ball collision detection
    // we can just do a quick 2d box/box hit test against the ball and each paddle
    // we'll just ignore that the ball is round for this.
    // also, since we know the paddles are always a fixed distance from the edges of
    // the window, we can exploit this and just check how close we are.
    if (((ball_pos[1] + (gameState_.ballSize() / 2.0f)) >= (left_.position() - paddle_mid)) &&
        ((ball_pos[1] - (gameState_.ballSize() / 2.0f)) <= (left_.position() + paddle_mid)) &&
        (ball_pos[0] <= ((gameState_.ballSize() / 2.0f) + paddle_size))) {
        // alter ball direction
        glm::vec2 ball_dir = ball_.direction();
        ball_dir = -ball_dir;
        if (left_.down())
            ball_dir += glm::vec2(0.0f, -0.015f);
        else if (left_.up())
            ball_dir -= glm::vec2(0.0f, -0.015f);
        // speed the ball up slightly
        ball_dir *= gameState_.ballSpeedup();

        ball_.direction(ball_dir);
        dispatcher_->trigger(v3d::event::Sound("hit"));
    } else if (((ball_pos[1] + (gameState_.ballSize() / 2.0f)) >= (right_.position() - paddle_mid)) &&
            ((ball_pos[1] - (gameState_.ballSize() / 2.0f)) <= (right_.position() + paddle_mid)) &&
            (ball_pos[0] >= (width_ - ((gameState_.ballSize() / 2.0f) + paddle_size)))) {
        // alter ball direction
        glm::vec2 ball_dir = ball_.direction();
        ball_dir = -ball_dir;
        if (right_.down())
            ball_dir += glm::vec2(0.0f, 0.015f);
        else if (right_.up())
            ball_dir -= glm::vec2(0.0f, 0.015f);
        // speed the ball up slightly
        ball_dir *= gameState_.ballSpeedup();

        ball_.direction(ball_dir);
        dispatcher_->trigger(v3d::event::Sound("hit"));
    }

    bool reset_ball = false;
    float victor = 0.0f;
    // if the ball hits the left or right edge of the screen then we need
    // to update the score and reset the ball
    if (ball_pos[0] <= (gameState_.ballSize() / 2.0f)) {
        right_.score(right_.score() + 1);
        victor = -1.0f;
        reset_ball = true;
        dispatcher_->trigger(v3d::event::Sound("score"));
    } else if (ball_pos[0] >= (width_ - (gameState_.ballSize() / 2.0f))) {
        left_.score(left_.score() + 1);
        victor = 1.0f;
        reset_ball = true;
        dispatcher_->trigger(v3d::event::Sound("score"));
    }
    if (reset_ball) {
        // reposition the ball in the center of the screen
        float mid_y = height_ / 2.0f;
        float mid_x = width_ / 2.0f;
        glm::vec2 v(mid_x, mid_y);
        ball_.position(v);
        // set the ball rolling
        float speed = gameState_.ballStartSpeed() * gameState_.ballSpeedup();
        // last winner serves the ball
        glm::vec2 dir(speed * victor, 0.0f);
        ball_.direction(dir);
        // start at this slightly faster speed next time
        gameState_.ballStartSpeed(speed);

        // reset the default paddle positions
        left_.position(mid_y);
        right_.position(mid_y);
    }

    // if the ball has hit the top or bottom of the screen then we need to alter the
    // direction of the ball so it bounces off
    if (ball_pos[1] >= ((height_ - 15.0f) - (gameState_.ballSize() / 2.0f))) {
        glm::vec2 ball_dir = ball_.direction();
        ball_dir[1] = -ball_dir[1];
        ball_.direction(ball_dir);
        dispatcher_->trigger(v3d::event::Sound("bounce"));
    } else if (ball_pos[1] <= (15.0f - (gameState_.ballSize() / 2.0f))) {
        glm::vec2 ball_dir = ball_.direction();
        ball_dir[1] = -ball_dir[1];
        ball_.direction(ball_dir);
        dispatcher_->trigger(v3d::event::Sound("bounce"));
    }


    float step = 1.5f;
    float bottom = 560.0f;
    float top = 40.0f;
    if (left_.up()) {
        /// FIXME: use "Uint32 SDL_GetTicks(void)" to work out a movement delta
        /// use variables for screen extents and paddle sizes
        if (left_.position() > top)
            left_.position(left_.position() - step);
    } else if (left_.down()) {
        if (left_.position() < bottom)
            left_.position(left_.position() + step);
    } else {
    }

    if (right_.up()) {
        if (right_.position() > top)
            right_.position(right_.position() - step);
    } else if (right_.down()) {
        if (right_.position() < bottom)
            right_.position(right_.position() + step);
    } else {
    }
    ball_.move();
}

void PongScene::reset() {
    float mid_y = height_ / 2.0f;
    float mid_x = width_ / 2.0f;
    // set the default paddle positions
    left_.position(mid_y);
    right_.position(mid_y);
    // set the position of the right paddle
    right_.offset(785.0f);

    // this is the ball's starting position
    glm::vec2 v(mid_x, mid_y);
    ball_.position(v);
    // ... and direction
    float speed = gameState_.ballStartSpeed();
    glm::vec2 dir(-speed, 0.0f);
    ball_.direction(dir);
    // ... and size
    ball_.size(gameState_.ballSize());

    // reset game state
    left_.reset();
    right_.reset();
    gameState_.reset();
    // hide menu and unpause
    // menu()->show(false);
    // pause(false);
}

Ball & PongScene::ball() {
    return ball_;
}

Paddle & PongScene::left() {
    return left_;
}

Paddle & PongScene::right() {
    return right_;
}

GameState & PongScene::state() {
    return gameState_;
}
