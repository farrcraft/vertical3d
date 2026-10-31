/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

#include "Ball.h"
#include "Paddle.h"
#include "GameState.h"

#include <entt/entt.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

class PongScene {
 public:
    /**
     * The court, in its own units. The rules are written against it, and the renderer maps it
     * onto a canvas with its own coordinate space, fitted to whatever the window is.
     **/
    static constexpr float width = 800.0F;
    static constexpr float height = 600.0F;
    /**
     * The thickness of the top and bottom walls, in court units. The ball turns at a wall's
     * face, and a paddle stops with its end against one.
     **/
    static constexpr float wall = 15.0F;

    explicit PongScene(entt::registry* registry, const boost::shared_ptr<entt::dispatcher> & dispatcher);
    ~PongScene();

    /**
     * Advance the rally by one simulation step.
     * @param step seconds, which every speed below is expressed against
     **/
    void tick(float step);

    void reset();

    /**
     * The paddle commands. Each step, the engine passes each one to steer() with whether its
     * key is held.
     **/
    static constexpr std::string_view paddleCommands[] = {
        "leftPaddleUp", "leftPaddleDown", "rightPaddleUp", "rightPaddleDown"
    };

    /**
     * Apply a paddle command: one of paddleCommands, with whether its key is held now. A held
     * key is ignored while the game is paused. A key that is not held stops its paddle, so a key
     * let go while the menu is up does not leave its paddle moving. Outside coop mode the
     * computer steers the right paddle, and its commands do nothing.
     *
     * @return whether the name was a paddle command
     **/
    bool steer(std::string_view command, bool held);

    /**
     * Change between coop and playing against the computer. Both paddles stop, so a paddle the
     * computer was moving does not keep moving once a player has it.
     **/
    void coop(bool mode);

    Ball & ball();
    Paddle & left();
    Paddle & right();
    GameState & state();

 private:
    // court units per second. The court is 800x600 and a paddle runs 40 to 560, so a paddle crosses
    // its whole run in a little under six seconds.
    static constexpr float PADDLE_SPEED = 90.0F;

    // the vertical velocity a travelling paddle adds to the return, in court units per second,
    // before the return is scaled back to the ball's incoming speed. It is a third of the
    // paddle's own speed, enough to steer a return without overriding where it was struck
    static constexpr float PADDLE_ENGLISH = 30.0F;

    // the steepest return, off either end of a paddle, in radians: fifty degrees from the
    // horizontal. A ball struck by the paddle's centre goes back flat
    static constexpr float MAX_RETURN_ANGLE = 0.8727F;

    /**
     * The pan of a point on the court: -1 at the left edge, 1 at the right. The court is drawn
     * with x running left to right across the window, so left on the court is left on screen.
     **/
    static float across(float x);

    /**
     * The phases of one tick, in the order tick() runs them. Each phase that takes a position
     * is given the one tick() read before the first phase. A bounce changes only the ball's
     * direction, and of the phases only scorePoint() moves the ball.
     **/
    void checkVictory();
    void steerOpponent(const glm::vec2& ballPosition);
    void bouncePaddles(const glm::vec2& ballPosition);
    /**
     * Send the ball back off a paddle, at an angle set by where along the paddle it struck.
     * @param away the sign of the horizontal direction away from that paddle
     **/
    void returnBall(Paddle& paddle, const glm::vec2& ballPosition, float away);
    void scorePoint(const glm::vec2& ballPosition);
    void bounceWalls(const glm::vec2& ballPosition);
    void movePaddles(float step);

    boost::shared_ptr<entt::dispatcher> dispatcher_;
    Ball ball_;
    Paddle left_, right_;
    GameState gameState_;
    entt::registry* registry_;
};
