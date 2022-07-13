/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "GameBoard.h"

class TetrisScene {
 public:
    TetrisScene();

    void tick(unsigned int delta);
    void resize(int width, int height);

    GameBoard * board();

    bool debug() const;
    void debug(bool dbg);

 private:
    GameBoard board_;
    int score_;
    bool debug_;
};
