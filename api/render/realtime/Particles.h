/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "DepthOrder.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::realtime {

/**
 * The particles of every entity carrying both an ecs::component::Emitter and a
 * component::Particles, added to a depth order as quads - ADR-0072.
 *
 * Each particle is drawn alpha of the way from its previous position to its position. Its key
 * is how far that lies along depthAxis, measured as sprites() measures a sprite's, so particles
 * and sprites handed to one order sort among each other.
 *
 * @param alpha Engine::alpha(), or one for emitters stepped once a frame
 * @param right the camera's right, which a particle's width runs along
 * @param up the camera's up
 * @param depthAxis the direction the key measures distance along
 **/
void particles(const entt::registry& registry, float alpha, const glm::vec3& right,
    const glm::vec3& up, const glm::vec3& depthAxis, DepthOrder* order);

};  // namespace v3d::render::realtime
