/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Profile.h>

#include <optional>
#include <span>

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::realtime::shadow {

/**
 * The smallest radius fit() returns, in world units. light() divides by the radius, so a fit
 * never returns zero or less.
 **/
inline constexpr float minimumRadius = 0.01f;

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
 * The eye is placed two radii out towards the light, and the far plane four radii beyond it.
 * The sphere therefore spans a quarter to three quarters of the depth range, and a caster just
 * outside it still lands in the map. The view is built as type::camera::Profile::lookat and
 * Camera::createView build a camera's in the hand given. A face is therefore wound the same way
 * under the light as under a camera of that hand, and one cull mode serves both passes.
 *
 * @param towards the direction towards the light - LitSettings::light. It need not be
 *        normalised, and must not be zero
 * @param centre the middle of what should cast and receive shadows
 * @param radius how far from the centre that reaches, which must be above zero
 * @param hand the hand of the camera the scene is drawn through
 **/
glm::mat4 light(const glm::vec3& towards, const glm::vec3& centre, float radius,
    type::camera::Profile::Hand hand = type::camera::Profile::Hand::UpCrossDirection);

/**
 * A sphere around every entity carrying an ecs::component::Transform and a
 * component::Mesh that casts a shadow, and the extra points given.
 *
 * The centre is the mean of the positions and the radius the farthest of them from it, plus
 * the margin. The radius is at least minimumRadius, which one caster with no margin, or a
 * negative margin, would otherwise take to zero or below. A position is an origin rather than
 * an extent, so the margin is what covers a caster's size and the length of the shadow it
 * throws. What casts nothing is left out, because a ground plane reaches past anything that
 * shadows it and would spread the map's texels over ground no shadow falls on.
 *
 * @param alsoCover points that should be inside the map whether or not anything stands there,
 *        such as where a character will walk to
 * @return nothing when no entity casts a shadow, so a caller keeps the bounds it had
 **/
std::optional<Bounds> fit(const entt::registry& registry, std::span<const glm::vec3> alsoCover, float margin);

};  // namespace v3d::render::realtime::shadow
