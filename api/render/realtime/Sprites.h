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
 * Every entity carrying both an ecs::component::Transform and a component::Sprite, added to a
 * depth order as an upright quad.
 *
 * The quad stands on the transform's position and is spanned by the camera's right and up, so
 * it faces the camera whatever way the entity is turned. Its width scales by the transform's x
 * scale and its height by y. An entity with a previous step is drawn alpha of the way from it.
 *
 * The key is how far the position lies along depthAxis, larger being further. The caller
 * chooses the axis: the camera's forward flattened onto the ground for an orthographic view of
 * a ground plane, or the view direction for a perspective one.
 *
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one
 * @param right the camera's right, which the quad's width runs along
 * @param up the camera's up, which the quad's height runs along
 * @param depthAxis the direction the key measures distance along
 **/
void sprites(const entt::registry& registry, float alpha, const glm::vec3& right,
    const glm::vec3& up, const glm::vec3& depthAxis, DepthOrder* order);

};  // namespace v3d::render::realtime
