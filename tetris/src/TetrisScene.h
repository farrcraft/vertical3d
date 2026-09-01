/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "GameBoard.h"

#include "../../api/asset/Manager.h"
#include "../../api/log/Logger.h"

#include <boost/shared_ptr.hpp>

class TetrisScene {
 public:
    explicit TetrisScene(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read what the board needs before it can be played.
     * @return false when the tetrad shapes could not be read
     */
    bool load(const boost::shared_ptr<v3d::asset::Manager>& assetManager);

    void tick(unsigned int delta);
    void resize(int width, int height);

    /**
     * Start a new game on an empty board.
     */
    void reset();

    GameBoard * board();

    /**
     * @return the score of the game in progress
     */
    unsigned int score() const;

    /**
     * Whether the game is running. A paused game still draws, but its board does not fall.
     */
    bool paused() const;
    void pause(bool paused);

    bool debug() const;
    void debug(bool dbg);

 private:
    GameBoard board_;
    bool paused_;
    bool debug_;
};
