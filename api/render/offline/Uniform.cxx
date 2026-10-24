/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Uniform.h"

#include <algorithm>
#include <cmath>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::uniform {

namespace {

constexpr float PI = 3.14159265358979F;

};  // namespace

glm::vec2 disc(const glm::vec2 & square) {
    // the square's diagonals cut the disc into four quarters, and this is half the angle of one
    constexpr float halfQuarter = PI / 4.0F;
    const glm::vec2 centred = square * 2.0F - glm::vec2(1.0F);
    if (centred.x == 0.0F && centred.y == 0.0F) {
        return glm::vec2(0.0F);
    }
    float radius = 0.0F;
    float angle = 0.0F;
    if (std::fabs(centred.x) > std::fabs(centred.y)) {
        radius = centred.x;
        angle = halfQuarter * (centred.y / centred.x);
    } else {
        radius = centred.y;
        angle = (PI / 2.0F) - (halfQuarter * (centred.x / centred.y));
    }
    return glm::vec2(radius * std::cos(angle), radius * std::sin(angle));
}

glm::vec3 triangle(const glm::vec3 & first, const glm::vec3 & second, const glm::vec3 & third, const glm::vec2 & square) {
    // the square root spreads the first number over the triangle's height evenly by area
    const float root = std::sqrt(square.x);
    const float towardSecond = root * (1.0F - square.y);
    const float towardThird = root * square.y;
    return (first * (1.0F - root)) + (second * towardSecond) + (third * towardThird);
}

glm::vec3 sphere(const glm::vec2 & square) {
    // a height spread evenly over [-1, 1] spreads points evenly over the sphere, by
    // Archimedes' hat-box theorem
    const float z = 1.0F - (2.0F * square.x);
    const float ring = std::sqrt(std::max(0.0F, 1.0F - (z * z)));
    const float angle = 2.0F * PI * square.y;
    return glm::vec3(ring * std::cos(angle), ring * std::sin(angle), z);
}

};  // namespace v3d::render::offline::uniform
