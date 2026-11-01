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

    void reset(void);

    int maxScore() const;
    bool coop() const;
    float ballSize() const;
    float ballStartSpeed() const;

    void coop(bool mode);
    void ballStartSpeed(float speed);
    void maxScore(int max);

    void pause(bool state);
    bool paused() const;

 private:
    // the side of the ball's box, in court units
    static constexpr float DEFAULT_BALL_SIZE = 10.0F;
    static constexpr int DEFAULT_MAX_SCORE = 5;
    // court units per second: a served ball crosses the 800 unit court in a little over
    // thirteen seconds
    static constexpr float BALL_START_SPEED = 60.0F;

    float ballSize_;
    float ballStartSpeed_;
    bool coop_;
    bool paused_;
    int maxScore_;
    SimulationState state_;
};
