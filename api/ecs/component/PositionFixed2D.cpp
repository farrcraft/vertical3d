/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "PositionFixed2D.h"


namespace v3d::ecs::component {

    PositionFixed2D::PositionFixed2D(const int x, const int y) noexcept
        : position_(x, y) {
    }

    PositionFixed2D::PositionFixed2D(PositionFixed2D&& p) noexcept {
        this->position_ = p.position_;
    }

    PositionFixed2D& PositionFixed2D::operator=(PositionFixed2D&& p) noexcept
    {
        if (this != &p) {
            this->position_ = p.position_;
        }

        return *this;
    }

    int PositionFixed2D::x() const {
        return position_.x;
    }

    int PositionFixed2D::y() const {
        return position_.y;
    }
};  // namespace v3d::ecs::component
