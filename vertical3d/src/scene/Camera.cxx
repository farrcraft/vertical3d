/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Camera.h"

#include "Item.h"

namespace v3d::editor {

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
}

};  // namespace v3d::editor
