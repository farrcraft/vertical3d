/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Length.h"

namespace v3d::ui {

Length::Length() noexcept :
    value_(0.0f),
    unit_(Unit::Auto) {
}

Length::Length(float value, Unit unit) noexcept :
    value_(value),
    unit_(unit) {
}

float Length::value() const noexcept {
    return value_;
}

Length::Unit Length::unit() const noexcept {
    return unit_;
}

float Length::resolve(float extent, float own) const noexcept {
    switch (unit_) {
        case Unit::Pixels:
            return value_;
        case Unit::Percent:
            return extent * value_ * 0.01f;
        case Unit::Auto:
        default:
            return own;
    }
}

};  // namespace v3d::ui
