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
 * What an emitter makes: how often, where, moving how, and looking how over a particle's life -
 * ADR-0072.
 *
 * The spawn shape and the direction are in the emitter's own space, which step() turns into
 * the world by an orientation, so a muzzle flash follows the way its gun faces. Acceleration is
 * in the world, since gravity and wind do not turn with what emits.
 **/
struct Emitter final {
    /**
     * Where a particle is born, about the emitter's origin.
     **/
    enum class Shape {
        Point,
        Sphere,   /**< anywhere inside a ball of radius extent.x **/
        Box       /**< anywhere inside a box of half size extent **/
    };

    float rate{ 0.0f };      /**< particles a second; burst() adds them all at once **/
    uint32_t cap{ 256 };     /**< how many may live at once; what is owed beyond it is dropped **/

    float lifeMin{ 1.0f };   /**< a particle's lifetime in seconds, drawn between these **/
    float lifeMax{ 1.0f };

    Shape shape{ Shape::Point };
    glm::vec3 extent{ 0.0f };

    glm::vec3 direction{ 0.0f, 1.0f, 0.0f };  /**< the centre of the cone a particle is launched in **/
    float spread{ 0.0f };    /**< the cone's half angle in radians: 0 is along direction, pi any way **/
    float speedMin{ 0.0f };  /**< launch speed in units a second, drawn between these **/
    float speedMax{ 0.0f };

    glm::vec3 acceleration{ 0.0f };  /**< gravity, a steady wind **/
    float drag{ 0.0f };      /**< the fraction of its velocity a particle loses a second **/

    /**
     * A sideways drift added where a particle is drawn rather than where it is simulated, a
     * snowflake's: so far either side along the camera's right, swaying so many times a second,
     * each particle at its own phase.
     **/
    float sway{ 0.0f };
    float swayRate{ 0.0f };

    /**
     * Size in world units and colour, each over a particle's life from 0 at birth to 1 at death.
     **/
    animation::Track<float> size{ 1.0f };
    animation::Track<glm::vec4> colour{ glm::vec4(1.0f) };
};

/**
 * Age, move and remove the particles an emitter has made, without spawning any.
 *
 * A particle whose life ends within the step is removed by swapping the last into its place, so
 * the order of particles is not the order they were born in.
 *
 * @param wind added to the emitter's acceleration for this step
 **/
void travel(const Emitter& emitter, State* state, float seconds, const glm::vec3& wind = glm::vec3(0.0f));

/**
 * How many whole particles a rate has earned over a step, keeping the fraction it has not.
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
 * Move the particles an emitter has made, then spawn what its rate owes within its shape.
 *
 * One born this step stands where it was born, with its previous position there too.
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
