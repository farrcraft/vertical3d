/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Particle.h"

#include <algorithm>

namespace v3d::type::effect {

float Particle::life() const noexcept {
    return lifetime > 0.0F ? std::clamp(age / lifetime, 0.0F, 1.0F) : 1.0F;
}

};  // namespace v3d::type::effect
