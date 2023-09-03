/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

class GameState {
 public:
    GameState();

    typedef enum {
        STATE_LOADING,
        STATE_SIMULATION,
        STATE_GUI
    } SimulationState;

    void reset(void);

    int maxScore() const;
    bool coop() const;
    float ballSize() const;
    float ballSpeedup() const;
    float ballStartSpeed() const;

    void coop(bool mode);
    void ballStartSpeed(float speed);
    void maxScore(int max);

    void pause(bool state);
    bool paused() const;

 private:
    float ballSize_;
    float ballSpeedup_;
    float ballStartSpeed_;
    bool coop_;
    bool paused_;
    int maxScore_;
    SimulationState state_;
};
