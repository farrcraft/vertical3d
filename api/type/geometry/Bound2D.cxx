/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Bound2D.h"

namespace v3d::type::geometry {

Bound2D::Bound2D(const Area& area) : size_(area.size), position_(area.position) {
}

bool Bound2D::contains(const glm::vec2& point) const {
    return (point[0] >= position_[0]) &&
        (point[1] >= position_[1]) &&
        (point[0] <= (position_[0] + size_[0])) &&
        (point[1] <= (position_[1] + size_[1]));
}

bool Bound2D::overlaps(const Bound2D& other) const {
    const glm::vec2 end = position_ + size_;
    const glm::vec2 otherEnd = other.position_ + other.size_;
    return (position_[0] <= otherEnd[0]) &&
        (other.position_[0] <= end[0]) &&
        (position_[1] <= otherEnd[1]) &&
        (other.position_[1] <= end[1]);
}

glm::vec2 Bound2D::size() const {
    return size_;
}

glm::vec2 Bound2D::position() const {
    return position_;
}

};  // namespace v3d::type::geometry
