/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Random.h>
#include "Particle.h"

#include <cstdint>
#include <vector>

namespace v3d::type::effect {

/**
 * What an emitter has made, and what it owes.
 **/
struct State final {
    /**
     * @param seed what every particle's randomness is drawn from, so a seed gives the same
     *        particles every time
     **/
    explicit State(uint64_t seed = 0) noexcept;

    std::vector<Particle> particles;
    float owed;            /**< the fraction of a particle the rate has earned and not yet spawned **/
    Random random;
};

};  // namespace v3d::type::effect
