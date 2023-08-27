/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <utility>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

struct Offset final {
    float offset_;
};

struct Score final {
    int score_;
};

struct Travel final {
    bool up_;
    bool down_;
};

struct PaddleSize final {
    float size_;
    float length_;
};

class Paddle final {
 public:
    Paddle(entt::registry& registry);

    void color(const glm::vec3 & color);
    void move(float delta);
    void position(const float pos);
    void offset(const float off);
    float offset() const;
    float position() const;

    glm::vec3 color() const;

    void reset();

    bool up();
    bool down();
    int score();
    float length();
    float size();

    void up(bool k);
    void down(bool k);
    void score(int s);

 private:
    entt::entity id_;
    entt::registry& registry_;
};
