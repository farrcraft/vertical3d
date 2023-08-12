/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Color3.h"

namespace v3d::ecs::component {

    Color3::Color3(const float red, const float blue, const float green) noexcept
        : color_(red, green, blue) {
    }

    Color3::Color3(Color3&& c) noexcept {
        this->color_ = c.color_;
    }

    Color3& Color3::operator=(Color3&& c) noexcept
    {
        if (this != &c) {
            this->color_ = c.color_;
        }

        return *this;
    }

    float Color3::red() const {
        return color_.x;
    }

    float Color3::green() const {
        return color_.y;
    }

    float Color3::blue() const {
        return color_.z;
    }
};  // namespace v3d::ecs::component
