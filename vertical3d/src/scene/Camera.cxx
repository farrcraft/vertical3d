/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Camera.h"

#include <cmath>

#include "Item.h"

namespace v3d::editor {

/**
 **/
bool Camera::Lens::valid() const {
    constexpr float straightAngle = 180.0F;
    return std::isfinite(fov) && std::isfinite(extent) && std::isfinite(clipping.x) && std::isfinite(clipping.y) &&
        fov > 0.0F && fov < straightAngle && extent > 0.0F && clipping.x > 0.0F && clipping.y > clipping.x;
}

/**
 **/
Camera::Camera() {
}

/**
 **/
Item::Kind Camera::kind() const noexcept {
    return KIND;
}

/**
 **/
const Camera::Lens& Camera::lens() const noexcept {
    return lens_;
}

/**
 **/
void Camera::lens(const Lens& lens) noexcept {
    lens_ = lens;
    changed();
}

};  // namespace v3d::editor
