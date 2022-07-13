/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <utility>

#include <glm/glm.hpp>

class Paddle {
 public:
    Paddle();

    void color(const glm::vec3 & color1, const glm::vec3 & color2);
    void move(float delta);
    void position(const float pos);
    void offset(const float off);
    float offset() const;
    float position() const;

    glm::vec3 color1() const;
    glm::vec3 color2() const;

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
    float position_;
    float offset_;
    glm::vec3 color1_;
    glm::vec3 color2_;

    float length_;
    float size_;

    bool up_;
    bool down_;
    int score_;
};
