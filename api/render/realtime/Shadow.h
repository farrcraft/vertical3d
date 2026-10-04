/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <span>

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::realtime::shadow {

/**
 * A sphere a directional light's shadow map is fitted to.
 **/
struct Bounds final {
    glm::vec3 centre{0.0f};
    float radius = 0.0f;
};

/**
 * The matrix a directional light draws its shadow map through: an orthographic box around a
 * sphere, seen from the light.
 *
 * The eye is placed two radii out towards the light, and the far plane four radii beyond it,
 * so the sphere spans a quarter to three quarters of the depth range and a caster just
 * outside it still lands in the map. The view is built as type::camera::Profile::lookat and
 * Camera::createProjection build a camera's, so a face is wound the same way under the light
 * as under the camera a scene is drawn through, and one cull mode serves both passes.
 *
 * @param towards the direction towards the light - LitSettings::light. It need not be
 *        normalised, and must not be zero
 * @param centre the middle of what should cast and receive shadows
 * @param radius how far from the centre that reaches, which must be above zero
 **/
glm::mat4 light(const glm::vec3& towards, const glm::vec3& centre, float radius);

/**
 * A sphere around every entity carrying an ecs::component::Transform and a
 * component::Mesh that casts a shadow, and the extra points given.
 *
 * The centre is the mean of the positions and the radius the farthest of them from it, plus
 * the margin. A position is an origin rather than an extent, so the margin is what covers a
 * caster's size and the length of the shadow it throws. What casts nothing is left out,
 * because a ground plane reaches past anything that shadows it and would spread the map's
 * texels over ground no shadow falls on.
 *
 * @param alsoCover points that should be inside the map whether or not anything stands there,
 *        such as where a character will walk to
 * @return nothing when no entity casts a shadow, so a caller keeps the bounds it had
 **/
std::optional<Bounds> fit(const entt::registry& registry, std::span<const glm::vec3> alsoCover, float margin);

};  // namespace v3d::render::realtime::shadow
