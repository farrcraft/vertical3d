/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>

class Ball {
 public:
    Ball();

    void direction(const glm::vec2 & dir);
    glm::vec2 direction() const;

    glm::vec2 position() const;
    void position(const glm::vec2 & pos);

    void move();

    float size() const;
    void size(float s);

 private:
    glm::vec2 position_;
    glm::vec2 direction_;
    float size_;
};
