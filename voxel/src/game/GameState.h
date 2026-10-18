/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cstdint>


class GameState {
 public:
    GameState();

    typedef enum : std::uint8_t {
        STATE_LOADING,
        STATE_SIMULATION,
        STATE_GUI
    } SimulationState;


    void pause(bool state);
    bool paused() const;

 private:
    SimulationState state_;
    bool paused_;
};
