/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Layout.h"

#include <api/type/geometry/Bound2D.h>

namespace v3d::ui {

Layout::Layout() noexcept :
    anchor(Anchor::TopLeft) {
}

v3d::type::geometry::Bound2D Layout::resolve(const v3d::type::geometry::Bound2D& parent, const glm::vec2& own) const {
    const glm::vec2 extent = parent.size();
    const glm::vec2 size(width.resolve({.extent = extent.x, .own = own.x}), height.resolve({.extent = extent.y, .own = own.y}));

    // an Auto position is no offset at all, so a component that names neither x nor y sits
    // at the corner it is anchored to
    const glm::vec2 offset(x.resolve({.extent = extent.x, .own = 0.0F}), y.resolve({.extent = extent.y, .own = 0.0F}));

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
            corner = parent.position() + (extent - size) * 0.5F + offset;
            break;
        case Anchor::TopLeft:
        default:
            break;
    }
    return v3d::type::geometry::Bound2D({.position = corner, .size = size});
}

};  // namespace v3d::ui
