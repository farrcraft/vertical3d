/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/DepthOrder.h>

#include <api/render/realtime/entity/Billboard.h>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::realtime::entity {

/**
 * The particles of every entity carrying both an ecs::component::Emitter and a
 * component::Particles, added to a depth order as quads.
 *
 * Each particle is drawn alpha of the way from its previous position to its position. Its width
 * runs along the billboard's right. Its key is how far it lies along the billboard's depth axis,
 * measured as sprites() measures a sprite's, so particles and sprites handed to one order sort
 * among each other.
 *
 * @param alpha Engine::alpha(), or one for emitters stepped once a frame
 **/
void particles(const entt::registry& registry, float alpha, const Billboard& billboard, canvas::DepthOrder* order);

};  // namespace v3d::render::realtime::entity
