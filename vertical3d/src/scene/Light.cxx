/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Light.h"

#include <cmath>

#include "Item.h"

#include <glm/gtc/constants.hpp>

namespace v3d::editor {

/**
 **/
bool Light::Emission::valid() const {
    const bool finite = std::isfinite(colour.r) && std::isfinite(colour.g) && std::isfinite(colour.b) &&
        std::isfinite(intensity) && std::isfinite(cone) && std::isfinite(delta);
    return finite && colour.r >= 0.0F && colour.g >= 0.0F && colour.b >= 0.0F && intensity >= 0.0F &&
        cone > 0.0F && cone < glm::pi<float>() && delta >= 0.0F && delta <= cone;
}

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
    changed();
}

};  // namespace v3d::editor
