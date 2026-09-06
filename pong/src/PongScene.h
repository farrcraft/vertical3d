/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Ball.h"
#include "Paddle.h"
#include "GameState.h"

#include <entt/entt.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

class PongScene {
 public:
    explicit PongScene(entt::registry* registry, const boost::shared_ptr<entt::dispatcher> & dispatcher);
    ~PongScene();

    /**
     * Advance the rally by one simulation step.
     * @param step seconds, which every speed below is expressed against
     **/
    void tick(float step);
    void resize(int width, int height);

    void reset();

    Ball & ball();
    Paddle & left();
    Paddle & right();
    GameState & state();

 private:
    /**
     * The phases of one tick, in the order tick() runs them. Each takes the ball position
     * tick() snapshotted rather than reading it again: a bounce changes the ball's
     * direction, and only scorePoint moves it.
     **/
    void checkVictory();
    void steerOpponent(const glm::vec2& ballPosition);
    void bouncePaddles(const glm::vec2& ballPosition);
    void scorePoint(const glm::vec2& ballPosition);
    void bounceWalls(const glm::vec2& ballPosition);
    void movePaddles(float step);

    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Ball ball_;
    Paddle left_, right_;
    GameState gameState_;
    int width_, height_;
    entt::registry* registry_;
};
