/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <utility>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

class Ball {
 public:
    explicit Ball(entt::registry* registry);

    void direction(const glm::vec2 & dir);
    glm::vec2 direction() const;

    glm::vec2 position() const;
    void position(const glm::vec2 & pos);

    /**
     * Where to draw the ball, alpha of the way from the last step to this one.
     **/
    glm::vec2 drawn(float alpha) const;

    /**
     * Make the next frame draw the ball at its position, with no motion from the last step.
     * Call it after placing the ball somewhere it did not move to.
     **/
    void settle();

    /**
     * Advance by the direction, which is a velocity in court units per second.
     * @param step seconds of simulated time
     **/
    void move(float step);

    float size() const;
    void size(float s);

 private:
    entt::registry* registry_;
    entt::entity id_;
};
