/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "GameState.h"

namespace {

// the side of the ball's box, in court units
constexpr float DEFAULT_BALL_SIZE = 10.0f;
constexpr int DEFAULT_MAX_SCORE = 5;
// court units per second: a served ball crosses the 800 unit court in a little over
// thirteen seconds
constexpr float BALL_START_SPEED = 60.0f;

};  // namespace

GameState::GameState() : ballSize_(DEFAULT_BALL_SIZE), coop_(true), paused_(false), maxScore_(DEFAULT_MAX_SCORE) {
    reset();
}

void GameState::pause(bool state) {
    paused_ = state;
}

bool GameState::paused() const {
    return paused_;
}

void GameState::reset() {
    ballStartSpeed_ = BALL_START_SPEED;
}

int GameState::maxScore() const {
    return maxScore_;
}

bool GameState::coop() const {
    return coop_;
}

float GameState::ballSize() const {
    return ballSize_;
}

float GameState::ballStartSpeed() const {
    return ballStartSpeed_;
}

void GameState::coop(bool mode) {
    coop_ = mode;
}

void GameState::ballStartSpeed(float speed) {
    ballStartSpeed_ = speed;
}

void GameState::maxScore(int max) {
    maxScore_ = max;
}
