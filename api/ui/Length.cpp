/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Length.h"

namespace v3d::ui {

namespace {

/**
 * What a percentage is multiplied by to give a fraction.
 **/
constexpr float PERCENT_TO_FRACTION = 0.01F;

};  // namespace

Length::Length() noexcept :
    value_(0.0F),
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

float Length::resolve(const Basis& basis) const noexcept {
    const float extent = basis.extent;
    const float own = basis.own;
    switch (unit_) {
        case Unit::Pixels:
            return value_;
        case Unit::Percent:
            return extent * value_ * PERCENT_TO_FRACTION;
        case Unit::Auto:
        default:
            return own;
    }
}

};  // namespace v3d::ui
