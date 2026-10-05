/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Layout.h"

namespace v3d::ui {

Layout::Layout() noexcept :
    anchor(Anchor::TopLeft) {
}

v3d::type::geometry::Bound2D Layout::resolve(const v3d::type::geometry::Bound2D& parent, const glm::vec2& own) const {
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
    return v3d::type::geometry::Bound2D(corner, size);
}

};  // namespace v3d::ui
