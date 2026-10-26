/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Profile.h>

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::lighting {

/**
 * The smallest radius fitShadow() returns, in world units. shadowLight() divides by the radius,
 * so a fit never returns zero or less.
 **/
inline constexpr float minimumShadowRadius = 0.01F;

/**
 * A sphere a directional light's shadow map is fitted to.
 **/
struct ShadowBounds final {
    glm::vec3 centre{0.0F};
    float radius = 0.0F;
};

/**
 * How fitShadow() reads the casters. The margin is added to the radius around them. alpha is
 * engine::Engine::alpha(), the fraction of a step elapsed since the last one. At one, the
 * current step's positions are read.
 **/
struct ShadowFit final {
    float margin = 0.0F;
    float alpha = 1.0F;
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
 * @param towards the direction towards the light, such as Settings::light. It need not be
 *        normalised, and must not be zero
 * @param bounds the middle of what should cast and receive shadows, and how far from it that
 *        reaches, which must be above zero
 * @param hand the hand of the camera the scene is drawn through
 **/
glm::mat4 shadowLight(const glm::vec3& towards, const ShadowBounds& bounds,
    type::camera::Profile::Hand hand = type::camera::Profile::Hand::UpCrossDirection);

/**
 * A sphere around every entity carrying an ecs::component::Transform and a
 * component::Mesh that casts a shadow, and the extra points given.
 *
 * The centre is the mean of the positions and the radius the farthest of them from it, plus
 * the margin. The radius is at least minimumShadowRadius, which one caster with no margin, or a
 * negative margin, would otherwise take to zero or below. A position is an origin, not an
 * extent. The margin covers a caster's size and the length of the shadow it throws. What
 * casts nothing is left out, because a ground plane reaches past anything that shadows it
 * and would spread the map's texels over ground no shadow falls on.
 *
 * Each position is the one drawn: alpha of the way from the caster's previous step to its
 * current one, as the lit pass draws it.
 *
 * @param alsoCover points that should be inside the map whether or not anything stands there,
 *        such as where a character will walk to
 * @return nothing when no entity casts a shadow, so a caller keeps the bounds it had
 **/
std::optional<ShadowBounds> fitShadow(const entt::registry& registry, std::span<const glm::vec3> alsoCover, const ShadowFit& fit);

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
 * @param towards the direction towards the light, as shadowLight() takes it. It must be finite
 *        and not zero
 * @param texels the shadow map's width in texels, at least 4
 * @return nothing when a value is out of range or the matrix cannot be inverted
 **/
std::optional<ShadowBounds> fitShadowToView(const glm::mat4& viewProjection, float distance, const glm::vec3& towards, uint32_t texels);

/**
 * The most cascades a shadow map has, which is how many SceneUniforms holds.
 **/
inline constexpr uint32_t maximumCascades = 4;

/**
 * Where each cascade of a shadow map ends, by the practical split scheme: each split is a
 * blend of a logarithmic one, which gives every cascade the same ratio of far to near, and a
 * uniform one, which gives every cascade the same length.
 *
 * A distance is measured from the camera's near plane along its view direction, as
 * fitCascades() takes it.
 *
 * @param near the camera's near plane, its distance from the eye, which must be above zero
 * @param distance how far from the near plane shadows are drawn, which must be above zero
 * @param count how many cascades, from 1 to maximumCascades
 * @param lambda 1 for logarithmic splits and 0 for uniform ones, and between them a blend.
 *        Held between 0 and 1
 * @return the far end of each cascade, nearest first, the last at distance. Empty when a value
 *         is out of range
 **/
std::vector<float> splitView(float near, float distance, uint32_t count, float lambda);

/**
 * How fitCascades() fits a shadow map's cascades.
 **/
struct CascadeFit final {
    /** each layer's width in texels, at least 4 **/
    uint32_t texels = 2048;
    /**
     * The part of each cascade's far distance over which it blends into the next. The next
     * cascade starts that much nearer, so both cover the band. Held between 0 and 0.5
     **/
    float blend = 0.1F;
    /** the hand of the camera the scene is drawn through, as shadowLight() takes it **/
    type::camera::Profile::Hand hand = type::camera::Profile::Hand::UpCrossDirection;
};

/**
 * A shadow map's cascades: a slice of the camera's view for each, and the matrix its layer is
 * drawn through. pack() writes them into SceneUniforms.
 **/
struct Cascades final {
    uint32_t count = 0;
    /** the matrix each cascade's shadow pass draws through, from shadowLight() **/
    std::array<glm::mat4, maximumCascades> lights{};
    /** the sphere each cascade covers, which entity::casters() culls against **/
    std::array<ShadowBounds, maximumCascades> bounds{};
    /** where each cascade ends, measured from the near plane, as the splits were given **/
    std::array<float, maximumCascades> far{};
    /**
     * The camera's near plane, as a normal along its view direction and an offset, so that
     * dot(xyz, p) + w is a point's distance from the near plane
     **/
    glm::vec4 plane{0.0F};
    float blend = 0.0F;
};

/**
 * Fit a cascade to each slice of the camera's view. Cascade i covers the view from the end of
 * cascade i - 1, less the blend, to splits[i], and the first starts at the near plane. Each is
 * fitted as fitShadowToView() fits one: a sphere whose radius holds still as the camera turns,
 * with its centre moved to a whole texel.
 *
 * @param viewProjection the camera's, mapping depth to 0 at the near plane and 1 at the far
 * @param splits where each cascade ends, measured from the near plane along the view
 *        direction, increasing and above zero, from 1 to maximumCascades of them. splitView()
 *        gives them, and a game can give its own
 * @param towards the direction towards the light, as shadowLight() takes it
 * @return nothing when a value is out of range or the matrix cannot be inverted
 **/
std::optional<Cascades> fitCascades(const glm::mat4& viewProjection, std::span<const float> splits, const glm::vec3& towards,
    const CascadeFit& fit = CascadeFit());

/**
 * Whether a caster can throw a shadow into a cascade: whether a sphere around it meets the box
 * shadowLight() draws the cascade through. A caster outside the box is clipped from that
 * cascade's layer anyway, so it need not be drawn into it.
 *
 * @param cascade the sphere the cascade covers
 * @param towards the direction towards the light, as shadowLight() takes it
 * @param caster a sphere around the caster, in world space
 **/
bool casts(const ShadowBounds& cascade, const glm::vec3& towards, const ShadowBounds& caster);

};  // namespace v3d::render::realtime::lighting
