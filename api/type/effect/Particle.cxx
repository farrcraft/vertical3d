/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Particle.h"

#include <algorithm>

namespace v3d::type::effect {

float Particle::life() const noexcept {
    return lifetime > 0.0f ? std::clamp(age / lifetime, 0.0f, 1.0f) : 1.0f;
}

};  // namespace v3d::type::effect
