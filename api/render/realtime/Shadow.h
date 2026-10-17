/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Profile.h>

#include <cstdint>
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
 * Each position is the one drawn: alpha of the way from the caster's previous step to its
 * current one, as the lit pass draws it.
 *
 * @param alsoCover points that should be inside the map whether or not anything stands there,
 *        such as where a character will walk to
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one. At one, the
 *        current step's positions are read
 * @return nothing when no entity casts a shadow, so a caller keeps the bounds it had
 **/
std::optional<Bounds> fit(const entt::registry& registry, std::span<const glm::vec3> alsoCover, float margin, float alpha = 1.0f);

/**
 * A sphere around what a camera sees, for a shadow map that follows the camera.
 *
 * The sphere encloses the camera's view volume from its near plane out to a distance along
 * each of its four edges, or to its far plane where that is nearer. Its radius depends only on
 * the shape of that volume, so it stays the same while the camera moves and turns.
 *
 * The centre is moved across the light to a whole number of the map's texels. A fixed point in
 * the world therefore falls at the same place within its texel from one frame to the next, and a
 * shadow's edge does not crawl as the camera moves. The radius includes room for that move.
 *
 * @param viewProjection the camera's, mapping depth to 0 at the near plane and 1 at the far
 * @param distance how far from the near plane shadows are drawn, which must be finite and
 *        positive
 * @param towards the direction towards the light, as light() takes it. It must be finite and
 *        not zero
 * @param texels the shadow map's width in texels, at least 4
 * @return nothing when a value is out of range or the matrix cannot be inverted
 **/
std::optional<Bounds> fitView(const glm::mat4& viewProjection, float distance, const glm::vec3& towards, uint32_t texels);

};  // namespace v3d::render::realtime::shadow
