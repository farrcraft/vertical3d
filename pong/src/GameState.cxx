/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "GameState.h"

GameState::GameState() : ballSize_(10.0f), coop_(true), maxScore_(5) {
    reset();
}

void GameState::pause(bool state) {
    paused_ = state;
}

bool GameState::paused() const {
    return paused_;
}

void GameState::reset() {
    // percentage to increase ball speed each round
    ballSpeedup_ = 1.0f;
    ballStartSpeed_ = 1.0f;
    paused_ = false;
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

float GameState::ballSpeedup() const {
    return ballSpeedup_;
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
