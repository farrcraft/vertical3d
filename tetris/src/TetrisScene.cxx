/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TetrisScene.h"

TetrisScene::TetrisScene(const boost::shared_ptr<v3d::log::Logger>& logger) :
    board_(logger), paused_(false), debug_(false) {
}

bool TetrisScene::load(const boost::shared_ptr<v3d::asset::Manager>& assetManager) {
    return board_.load(assetManager);
}

void TetrisScene::resize(int width, int height) {
}

GameBoard * TetrisScene::board() {
    return &board_;
}

void TetrisScene::reset() {
    board_.reset();
    paused_ = false;
}

void TetrisScene::tick(unsigned int delta) {
    if (paused_) {
        return;
    }
    board_.update(delta);
}

unsigned int TetrisScene::score() const {
    return board_.score();
}

bool TetrisScene::paused() const {
    return paused_;
}

void TetrisScene::pause(bool paused) {
    paused_ = paused;
}

bool TetrisScene::debug() const {
    return debug_;
}

void TetrisScene::debug(bool dbg) {
    debug_ = dbg;
}
