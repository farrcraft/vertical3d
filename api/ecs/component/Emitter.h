/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ecs/effect/Emitter.h>
#include <api/ecs/effect/State.h>

#include <cstdint>

#include <entt/entt.hpp>

namespace v3d::ecs::component {

/**
 * A particle emitter on an entity. description says what particles it makes, and state holds
 * the particles it has made.
 *
 * emit() steps it from the entity's Transform, so its particles start where the entity stands.
 * A game does not snapshot an Emitter, because each particle keeps its own previous position.
 * The game sets how many particles it emits by writing description.rate, or by calling
 * ecs::effect::burst().
 **/
struct Emitter final {
    /**
     * @param seed the seed for the emitter's random numbers. The same seed gives the same
     *        particles
     **/
    explicit Emitter(uint64_t seed = 0);

    ecs::effect::Emitter description;
    ecs::effect::State state;
};

/**
 * Step every entity's emitter from its Transform's position and rotation. A game calls it from
 * simulate() with the simulation step, or from tick() when it draws its effects without
 * interpolating them. An emitter on an entity with no Transform stands at the origin.
 **/
void emit(entt::registry& registry, float step);

};  // namespace v3d::ecs::component
