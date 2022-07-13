/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TetrisScene.h"

#include <iostream>

TetrisScene::TetrisScene() : debug_(false) {
}

void TetrisScene::resize(int width, int height) {
}

GameBoard * TetrisScene::board() {
    return &board_;
}

void TetrisScene::tick(unsigned int delta) {
    board_.update(delta);
}

bool TetrisScene::debug() const {
    return debug_;
}

void TetrisScene::debug(bool dbg) {
    debug_ = dbg;
}
