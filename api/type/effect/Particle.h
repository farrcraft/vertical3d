/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::type::effect {

/**
 * One particle. Positions are in the world, so a particle stays where it was born when what
 * emitted it moves on.
 **/
struct Particle final {
    glm::vec3 position;
    glm::vec3 previous;  /**< where the previous step left it; a frame interpolates from here **/
    glm::vec3 velocity;
    float age;           /**< seconds since birth **/
    float lifetime;
    float phase;         /**< in [0, 1), the particle's own offset into anything periodic about it **/

    /**
     * The particle's tracks are sampled at this value.
     *
     * @return how far through its life the particle is, from 0 to 1. A lifetime that is not
     *         positive gives 1.
     **/
    float life() const noexcept;
};

};  // namespace v3d::type::effect
