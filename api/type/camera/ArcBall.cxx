/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "ArcBall.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace v3d::type::camera {

ArcBall::ArcBall() {
}

ArcBall::~ArcBall() {
}

glm::vec3 ArcBall::map(const glm::vec2& new_point) {
    glm::vec3 mapped_point;
    glm::vec2 point;
    point = new_point;

    // scale point to range [-1, 1]
    point[0] = (point[0] * width_) - 1.0F;
    point[1] = 1.0F - (point[1] * height_);

    float length;
    // squared length
    length = (point[0] * point[0]) + (point[1] * point[1]);

    // point mapped outside sphere (length > radius^2)
    if (length > 1.0F) {
        float norm;
        norm = 1.0F / sqrtf(length);

        mapped_point[0] = point[0] * norm;
        mapped_point[1] = point[1] * norm;
        mapped_point[2] = 0.0F;
    } else {  // point mapped inside sphere
        mapped_point[0] = point[0];
        mapped_point[1] = point[1];
        mapped_point[2] = sqrtf(1.0F - length);
    }
    return mapped_point;
}

void ArcBall::click(const glm::vec2& point) {
    start_ = map(point);
}

glm::quat ArcBall::drag(const glm::vec2& point) {
    end_ = map(point);

    const glm::vec3 perp = glm::cross(start_, end_);
    // glm::quat takes w first, then x, y and z
    const glm::quat rot = glm::length(perp) > glm::epsilon<float>()
        ? glm::quat(glm::dot(start_, end_), perp.x, perp.y, perp.z)
        : glm::quat(0.0F, 0.0F, 0.0F, 0.0F);
    start_ = end_;
    return rot;
}

void ArcBall::bounds(float width, float height) {
    // the scale below divides by (extent - 1). An extent of 1 would make every mapped
    // point a NaN, so each extent is at least 2.
    width = std::max(width, 2.0F);
    height = std::max(height, 2.0F);
    width_ = 1.0F / ((width - 1.0F) * 0.5F);
    height_ = 1.0F / ((height - 1.0F) * 0.5F);
}

};  // namespace v3d::type::camera
