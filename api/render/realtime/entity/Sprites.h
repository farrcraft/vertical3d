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
 * Every entity carrying both an ecs::component::Transform and a component::Sprite, added to a
 * depth order as an upright quad.
 *
 * The quad stands on the transform's position and is spanned by the billboard's right and up,
 * so it faces the camera whatever way the entity is turned. Its width scales by the transform's
 * x scale and its height by y. An entity with a previous step is drawn alpha of the way from it.
 * Its key is how far the position lies along the billboard's depth axis.
 *
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one
 **/
void sprites(const entt::registry& registry, float alpha, const Billboard& billboard, canvas::DepthOrder* order);

};  // namespace v3d::render::realtime::entity
