/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Position2D.h"


namespace v3d::ecs::component {

    Position2D::Position2D(const float x, const float y) noexcept
        : position_(x, y) {
    }

    Position2D::Position2D(Position2D&& p) noexcept {
        this->position_ = p.position_;
    }

    Position2D& Position2D::operator=(Position2D&& p) noexcept
    {
        if (this != &p) {
            this->position_ = p.position_;
        }

        return *this;
    }

    float Position2D::x() const {
        return position_.x;
    }

    float Position2D::y() const {
        return position_.y;
    }

    /**
     **/
    glm::vec2 Position2D::value() const {
        return position_;
    }

    /**
     **/
    void Position2D::set(const glm::vec2& position) {
        position_ = position;
    }

};  // namespace v3d::ecs::component
