/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <utility>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

class Paddle final {
 public:
    Paddle(entt::registry* registry);

    void color(const glm::vec3 & color);
    void move(float delta);
    void position(float pos);
    void offset(float off);
    float offset() const;
    float position() const;

    /**
     * Where to draw the paddle, alpha of the way from the last step to this one.
     **/
    float drawn(float alpha) const;

    /**
     * Draw the paddle where it is with no motion, after it has been put somewhere rather
     * than moved there.
     **/
    void settle();

    glm::vec3 color() const;

    void reset();

    bool up();
    bool down();
    int score();
    float length() const;
    float size() const;

    void up(bool k);
    void down(bool k);
    void score(int s);

 private:
    entt::entity id_;
    entt::registry* registry_;
};
