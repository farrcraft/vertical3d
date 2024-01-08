/**
 * Vertical3D
 * Copyright(c) 2024 Joshua Farr(josh@farrcraft.com)
**/

#include "GameState.h"

GameState::GameState() {
}

void GameState::pause(bool state) {
    paused_ = state;
}

bool GameState::paused() const {
    return paused_;
}

