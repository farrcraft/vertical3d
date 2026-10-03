/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Position1D.h"

#include <glm/common.hpp>

namespace v3d::ecs::component {

Position1D::Position1D(const float x) noexcept
    : position_(x) {
}

Position1D::Position1D(Position1D&& p) noexcept {
    this->position_ = p.position_;
}

Position1D& Position1D::operator=(Position1D&& p) noexcept {
    if (this != &p) {
        this->position_ = p.position_;
    }

    return *this;
}

float Position1D::x() const {
    return position_;
}


/**
 **/
float Position1D::value() const {
    return position_;
}

/**
 **/
void Position1D::set(float pos) {
    position_ = pos;
}

Position1D interpolate(const Position1D& from, const Position1D& to, float alpha) {
    return Position1D(glm::mix(from.value(), to.value(), alpha));
}

};  // namespace v3d::ecs::component
