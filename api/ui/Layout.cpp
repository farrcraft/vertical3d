/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Layout.h"

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

Layout::Layout() noexcept :
    anchor(Anchor::TopLeft) {
}

v3d::type::Bound2D Layout::resolve(const v3d::type::Bound2D& parent, const glm::vec2& own) const {
    const glm::vec2 extent = parent.size();
    const glm::vec2 size(width.resolve(extent.x, own.x), height.resolve(extent.y, own.y));

    // an Auto position is no offset at all, so a component that names neither x nor y sits
    // at the corner it is anchored to - ADR-0039
    const glm::vec2 offset(x.resolve(extent.x, 0.0f), y.resolve(extent.y, 0.0f));

    glm::vec2 corner = parent.position() + offset;
    switch (anchor) {
        case Anchor::TopRight:
            corner.x = parent.position().x + extent.x - offset.x - size.x;
            break;
        case Anchor::BottomLeft:
            corner.y = parent.position().y + extent.y - offset.y - size.y;
            break;
        case Anchor::BottomRight:
            corner.x = parent.position().x + extent.x - offset.x - size.x;
            corner.y = parent.position().y + extent.y - offset.y - size.y;
            break;
        case Anchor::Centre:
            corner = parent.position() + (extent - size) * 0.5f + offset;
            break;
        case Anchor::TopLeft:
        default:
            break;
    }
    return v3d::type::Bound2D(corner, size);
}

};  // namespace v3d::ui
