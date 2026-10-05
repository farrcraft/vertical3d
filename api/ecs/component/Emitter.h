/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/effect/Emitter.h>
#include <api/type/effect/State.h>

#include <cstdint>

#include <entt/entt.hpp>

namespace v3d::ecs::component {

/**
 * An entity that makes particles, and the particles it has made - ADR-0072.
 *
 * Stepped by emit() from the entity's Transform, so an effect carried by an entity is born where
 * the entity stands. It is not snapshotted: each particle keeps its own previous position.
 * When it fires and how hard is the game's, which writes description.rate or calls
 * type::effect::burst().
 **/
struct Emitter final {
    /**
     * @param seed what the emitter's particles are drawn from
     **/
    explicit Emitter(uint64_t seed = 0);

    type::effect::Emitter description;
    type::effect::State state;
};

/**
 * Step every entity's emitter from where its Transform stands and how it is turned. Called
 * from simulate() with the fixed step, or from tick() by a game that draws its effects at an
 * alpha of one. An emitter on an entity with no Transform stands at the origin.
 **/
void emit(entt::registry& registry, float step);

};  // namespace v3d::ecs::component
