/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Light.h"

#include "Item.h"

namespace v3d::editor {

/**
 **/
Light::Light() {
}

/**
 **/
Item::Kind Light::kind() const noexcept {
    return KIND;
}

/**
 **/
const Light::Emission& Light::emission() const noexcept {
    return emission_;
}

/**
 **/
void Light::emission(const Emission& emission) noexcept {
    emission_ = emission;
}

};  // namespace v3d::editor
