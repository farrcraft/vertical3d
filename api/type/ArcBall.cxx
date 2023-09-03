/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "ArcBall.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

namespace v3d::type {

    ArcBall::ArcBall() {
    }

    ArcBall::~ArcBall() {
    }

    glm::vec3 ArcBall::map(const glm::vec2& new_point) {
        glm::vec3 mapped_point;
        glm::vec2 point;
        point = new_point;

        // scale point to range [-1, 1]
        point[0] = (point[0] * width_) - 1.0f;
        point[1] = 1.0f - (point[1] * height_);

        float length;
        // squared length
        length = (point[0] * point[0]) + (point[1] * point[1]);

        // point mapped outside sphere (length > radius^2)
        if (length > 1.0f) {
            float norm;
            norm = 1.0f / sqrtf(length);

            mapped_point[0] = point[0] * norm;
            mapped_point[1] = point[1] * norm;
            mapped_point[2] = 0.0f;
        } else {  // point mapped inside sphere
            mapped_point[0] = point[0];
            mapped_point[1] = point[1];
            mapped_point[2] = sqrtf(1.0f - length);
        }
        return mapped_point;
    }

    void ArcBall::click(const glm::vec2& point) {
        start_ = map(point);
    }

    glm::quat ArcBall::drag(const glm::vec2& point) {
        end_ = map(point);

        glm::quat rot;
        glm::vec3 perp;

        perp = glm::cross(start_, end_);

        if (glm::length(perp) > glm::epsilon<float>()) {
            rot[0] = perp[0];
            rot[1] = perp[1];
            rot[2] = perp[2];
            rot[3] = glm::dot(start_, end_);
        } else {
            rot[0] = rot[1] = rot[2] = rot[3] = 0.0f;
        }
        start_ = end_;
        return rot;
    }

    void ArcBall::bounds(float width, float height) {
        if (width < 1.0f)
            width = 1.0f;
        if (height < 1.0f)
            height = 1.0f;
        width_ = 1.0f / ((width - 1.0f) * 0.5f);
        height_ = 1.0f / ((height - 1.0f) * 0.5f);
    }

};  // namespace v3d::type
