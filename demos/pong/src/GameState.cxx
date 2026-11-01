/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "GameState.h"

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
