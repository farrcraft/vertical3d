/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Ball.h"
#include "Paddle.h"
#include "GameState.h"

#include <entt/entt.hpp>
#include <boost/shared_ptr.hpp>

class PongScene {
 public:
    explicit PongScene(entt::registry& registry, const boost::shared_ptr<entt::dispatcher> & dispatcher);
    ~PongScene();

    void tick();
    void resize(int width, int height);

    void reset();

    Ball & ball();
    Paddle & left();
    Paddle & right();
    GameState & state();

 private:
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Ball ball_;
    Paddle left_, right_;
    GameState gameState_;
    int width_, height_;
    entt::registry& registry_;
};
