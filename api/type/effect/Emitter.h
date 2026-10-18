/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/animation/Track.h>
#include "State.h"

#include <cstdint>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::type::effect {

/**
 * The description of an emitter: how often it spawns particles, where, how they move, and how
 * they look over their life.
 *
 * The spawn shape and the direction are in the emitter's own space. step() turns them into the
 * world by an orientation, so a muzzle flash follows the way its gun faces. Acceleration is in
 * world space, because gravity and wind do not turn with the emitter.
 **/
struct Emitter final {
    /**
     * Where a particle is born, about the emitter's origin.
     **/
    enum class Shape : std::uint8_t {
        Point,
        Sphere,   /**< anywhere inside a ball of radius extent.x **/
        Box       /**< anywhere inside a box of half size extent **/
    };

    float rate{ 0.0F };      /**< particles a second that step() spawns. burst() ignores it **/
    uint32_t cap{ 256 };     /**< how many may live at once; particles due beyond it are dropped **/

    float lifeMin{ 1.0F };   /**< a particle's lifetime in seconds, drawn between these **/
    float lifeMax{ 1.0F };

    Shape shape{ Shape::Point };
    glm::vec3 extent{ 0.0F };

    glm::vec3 direction{ 0.0F, 1.0F, 0.0F };  /**< the centre of the cone a particle is launched in **/
    float spread{ 0.0F };    /**< the cone's half angle in radians: 0 is along direction, pi any way **/
    float speedMin{ 0.0F };  /**< launch speed in units a second, drawn between these **/
    float speedMax{ 0.0F };

    glm::vec3 acceleration{ 0.0F };  /**< gravity, a steady wind **/
    float drag{ 0.0F };      /**< the fraction of its velocity a particle loses a second **/

    /**
     * A sideways drift, like a snowflake's. It is added when a particle is drawn, not when it
     * is simulated. sway is how far the drift reaches either side along the camera's right,
     * and swayRate is how many times a second it sways. Each particle sways at its own phase.
     **/
    float sway{ 0.0F };
    float swayRate{ 0.0F };

    /**
     * Size in world units and colour, each over a particle's life from 0 at birth to 1 at death.
     **/
    animation::Track<float> size{ 1.0F };
    animation::Track<glm::vec4> colour{ glm::vec4(1.0F) };
};

/**
 * Age, move and remove the particles an emitter has made, without spawning any.
 *
 * A particle whose life ends within the step is removed by swapping the last into its place, so
 * the order of particles is not the order they were born in.
 *
 * @param wind added to the emitter's acceleration for this step
 **/
void travel(const Emitter& emitter, State* state, float seconds, const glm::vec3& wind = glm::vec3(0.0F));

/**
 * How many whole particles a rate has earned over a step. The leftover fraction stays in
 * state->owed for the next step.
 **/
uint32_t owing(State* state, float rate, float seconds);

/**
 * Add one particle at a place in the world, launched as the emitter launches, unless the
 * emitter is at its cap.
 *
 * @param orientation how the emitter is turned, which turns the launch direction
 * @return whether there was room for it
 **/
bool spawn(const Emitter& emitter, State* state, const glm::vec3& position, const glm::quat& orientation);

/**
 * Move the particles an emitter has made, then spawn the particles its rate makes due within
 * its shape.
 *
 * A particle born this step has both its position and its previous position at its birthplace.
 *
 * @param origin where the emitter stands in the world
 * @param orientation how the emitter is turned, which turns its spawn shape and direction
 * @param seconds how far to step
 **/
void step(const Emitter& emitter, State* state, const glm::vec3& origin, const glm::quat& orientation, float seconds);

/**
 * Spawn a number of particles at once within the emitter's shape, up to its cap.
 **/
void burst(const Emitter& emitter, State* state, const glm::vec3& origin, const glm::quat& orientation, uint32_t count);

};  // namespace v3d::type::effect
