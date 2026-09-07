/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bar.h"

#include <algorithm>

namespace v3d::ui::component {

Bar::Bar() :
    Component(Type::BAR),
    fraction_(0.0f),
    direction_(Direction::Horizontal) {
}

void Bar::fraction(float fill) {
    fraction_ = std::clamp(fill, 0.0f, 1.0f);
}

float Bar::fraction() const noexcept {
    return fraction_;
}

void Bar::direction(Direction fill) {
    direction_ = fill;
}

Bar::Direction Bar::direction() const noexcept {
    return direction_;
}

};  // namespace v3d::ui::component
