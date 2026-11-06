/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

class GameState {
 public:
    GameState();

    void pause(bool state);
    bool paused() const;

 private:
    bool paused_;
};
