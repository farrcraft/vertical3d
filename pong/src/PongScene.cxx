/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongScene.h"

#include <api/event/kind/Sound.h>
#include <api/type/geometry/Bound2D.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include <glm/geometric.hpp>

namespace {

// court units per second. The court is 800x600 and a paddle runs 40 to 560, so a paddle crosses
// its whole run in a little under six seconds.
constexpr float PADDLE_SPEED = 90.0f;

// the vertical velocity a travelling paddle adds to the return, in pixels per second - a third
// of the paddle's own speed, enough to steer a return without overriding where it was struck
constexpr float PADDLE_ENGLISH = 30.0f;

// the steepest return, off either end of a paddle, in radians: fifty degrees from the
// horizontal. A ball struck by the paddle's centre goes back flat
constexpr float MAX_RETURN_ANGLE = 0.8727f;

// the top and bottom walls' thickness, which the renderer draws at the same size
constexpr float WALL = 15.0f;

/**
 * The area a paddle returns the ball from: its own length vertically, and from left to right
 * horizontally, which the caller sets from the paddle's face to beyond the court's edge.
 **/
v3d::type::geometry::Bound2D paddleReach(Paddle& paddle, float left, float right) {
    const float top = paddle.position() - paddle.length() / 2.0f;
    return v3d::type::geometry::Bound2D(left, top, right - left, paddle.length());
}

};  // namespace

PongScene::PongScene(entt::registry* registry, const boost::shared_ptr<entt::dispatcher>& dispatcher) :
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

void PongScene::checkVictory() {
    if (left_.score() == gameState_.maxScore() ||
        right_.score() == gameState_.maxScore()) {
        reset();
        dispatcher_->trigger(v3d::event::kind::Sound("victory"));
    }
}

bool PongScene::steer(std::string_view command, bool held) {
    const bool left = command == "leftPaddleUp" || command == "leftPaddleDown";
    const bool right = command == "rightPaddleUp" || command == "rightPaddleDown";
    if (!left && !right) {
        return false;
    }
    if (held && (gameState_.paused() || (right && !gameState_.coop()))) {
        return true;
    }
    Paddle & paddle = left ? left_ : right_;
    if (command == "leftPaddleUp" || command == "rightPaddleUp") {
        paddle.up(held);
    } else {
        paddle.down(held);
    }
    return true;
}

void PongScene::coop(bool mode) {
    gameState_.coop(mode);
    for (Paddle * paddle : { &left_, &right_ }) {
        paddle->up(false);
        paddle->down(false);
    }
}

void PongScene::steerOpponent(const glm::vec2& ballPosition) {
    // give AI a turn in single player mode
    if (gameState_.coop()) {
        return;
    }
    glm::vec2 ball_dir = ball_.direction();
    // is the ball headed towards the ai's paddle (towards the right side)?
    if (ball_dir[0] > 0.0f) {
        // travel is signed the way the court is: up decreases the paddle position and
        // down increases it, so approaching a ball above the paddle is up.
        if (ballPosition[1] < right_.position()) {
            right_.up(true);
            right_.down(false);
        } else if (ballPosition[1] > right_.position()) {
            right_.up(false);
            right_.down(true);
        }
    } else {
        // no need to move the paddle if the ball is moving away from it
        right_.up(false);
        right_.down(false);
    }
}

void PongScene::bouncePaddles(const glm::vec2& ballPosition) {
    // the ball is treated as its square box, and each paddle as a box running from its face
    // out past the court's edge rather than as thick as the paddle. A ball that reaches the
    // face bounces, and so does one fast enough to have passed it between two steps. A box
    // the paddle's own width would let that ball tunnel through
    const float ballSize = gameState_.ballSize();
    const v3d::type::geometry::Bound2D ball(ballPosition - glm::vec2(ballSize / 2.0f), glm::vec2(ballSize));
    const float court = width;
    const float paddleSize = left_.size();
    const glm::vec2 direction = ball_.direction();

    // only a ball heading for the paddle is returned, so one still overlapping it on the step
    // after a bounce is not turned back again
    if (direction.x < 0.0f && ball.overlaps(paddleReach(left_, -court, paddleSize))) {
        returnBall(left_, ballPosition, 1.0f);
    } else if (direction.x > 0.0f && ball.overlaps(paddleReach(right_, court - paddleSize, 2.0f * court))) {
        returnBall(right_, ballPosition, -1.0f);
    }
}

void PongScene::returnBall(Paddle& paddle, const glm::vec2& ballPosition, float away) {
    // where along the paddle the ball met it, from -1 at the top end to 1 at the bottom. The
    // ball's own half size is included, so a ball clipping the very end counts as the end
    const float reach = paddle.length() / 2.0f + gameState_.ballSize() / 2.0f;
    const float along = std::clamp((ballPosition.y - paddle.position()) / reach, -1.0f, 1.0f);

    const float angle = along * MAX_RETURN_ANGLE;
    const float speed = glm::length(ball_.direction()) * gameState_.ballSpeedup();
    glm::vec2 returned(away * speed * std::cos(angle), speed * std::sin(angle));

    // a travelling paddle carries the ball along the way it is going: up is towards smaller y
    if (paddle.up()) {
        returned.y -= PADDLE_ENGLISH;
    } else if (paddle.down()) {
        returned.y += PADDLE_ENGLISH;
    }

    ball_.direction(returned);
    dispatcher_->trigger(v3d::event::kind::Sound("hit"));
}

void PongScene::scorePoint(const glm::vec2& ballPosition) {
    bool reset_ball = false;
    float victor = 0.0f;
    // if the ball hits the left or right edge of the screen then we need
    // to update the score and reset the ball
    if (ballPosition[0] <= (gameState_.ballSize() / 2.0f)) {
        right_.score(right_.score() + 1);
        victor = -1.0f;
        reset_ball = true;
        dispatcher_->trigger(v3d::event::kind::Sound("score"));
    } else if (ballPosition[0] >= (width - (gameState_.ballSize() / 2.0f))) {
        left_.score(left_.score() + 1);
        victor = 1.0f;
        reset_ball = true;
        dispatcher_->trigger(v3d::event::kind::Sound("score"));
    }
    if (!reset_ball) {
        return;
    }
    // reposition the ball in the center of the screen
    const float mid_y = height / 2.0f;
    const float mid_x = width / 2.0f;
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

    // all three were put back rather than moved, so none is drawn travelling there
    ball_.settle();
    left_.settle();
    right_.settle();
}

void PongScene::bounceWalls(const glm::vec2& ballPosition) {
    // a ball whose edge has reached a wall's face is sent away from that wall. Setting the
    // sign rather than flipping it means a ball still inside the wall on the next step is not
    // turned back into it
    const float half = gameState_.ballSize() / 2.0f;
    glm::vec2 direction = ball_.direction();
    const bool intoTop = ballPosition.y - half <= WALL && direction.y < 0.0f;
    const bool intoBottom = ballPosition.y + half >= height - WALL && direction.y > 0.0f;
    if (!intoTop && !intoBottom) {
        return;
    }
    direction.y = -direction.y;
    ball_.direction(direction);
    dispatcher_->trigger(v3d::event::kind::Sound("bounce"));
}

void PongScene::movePaddles(float step) {
    // a paddle stops with its end against a wall
    const float travel = PADDLE_SPEED * step;
    const float top = WALL + left_.length() / 2.0f;
    const float bottom = height - top;
    if (left_.up()) {
        if (left_.position() > top)
            left_.position(left_.position() - travel);
    } else if (left_.down()) {
        if (left_.position() < bottom)
            left_.position(left_.position() + travel);
    }

    if (right_.up()) {
        if (right_.position() > top)
            right_.position(right_.position() - travel);
    } else if (right_.down()) {
        if (right_.position() < bottom)
            right_.position(right_.position() + travel);
    }
}

void PongScene::tick(float step) {
    if (gameState_.paused()) {
        return;
    }

    checkVictory();

    // one snapshot serves the whole tick: a bounce changes the ball's direction rather
    // than its position, and the reposition a point scores happens after everything that
    // reads where the ball was
    const glm::vec2 ballPosition = ball_.position();

    steerOpponent(ballPosition);
    bouncePaddles(ballPosition);
    scorePoint(ballPosition);
    bounceWalls(ballPosition);
    movePaddles(step);

    ball_.move(step);
}


void PongScene::reset() {
    const float mid_y = height / 2.0f;
    const float mid_x = width / 2.0f;
    // set the default paddle positions
    left_.position(mid_y);
    right_.position(mid_y);
    // the right paddle stands its own width in from the far side
    right_.offset(width - right_.size());

    // this is the ball's starting position
    glm::vec2 v(mid_x, mid_y);
    ball_.position(v);
    // ... and direction
    float speed = gameState_.ballStartSpeed();
    glm::vec2 dir(-speed, 0.0f);
    ball_.direction(dir);
    // ... and size
    ball_.size(gameState_.ballSize());

    ball_.settle();
    left_.settle();
    right_.settle();

    // reset game state
    left_.reset();
    right_.reset();
    gameState_.reset();
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
