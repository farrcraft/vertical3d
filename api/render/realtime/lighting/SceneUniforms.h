/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Settings.h"
#include "Shadow.h"

#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::lighting {

/**
 * Set 2, binding 0, laid out as shaders/lit/lit.glsl declares the Scene block. std140 puts
 * every member here at its natural offset, so the struct maps straight across.
 **/
struct SceneUniforms final {
    glm::vec4 light;                /**< xyz towards the key light, normalised; w the fill **/
    glm::vec4 thresholds;           /**< x shadow to mid, y mid to lit **/
    glm::vec4 bands;                /**< xyz the shadow, mid and lit multipliers **/
    /** what each cascade's layer of the shadow map was drawn through **/
    std::array<glm::mat4, maximumCascades> lightViewProjection;
    glm::vec4 shadow;               /**< x a shadow map texel in uv, y strength, z normal bias **/
    glm::vec4 colour;               /**< xyz the light's colour, over the mid and lit bands **/
    glm::vec4 shadowColour;         /**< xyz the shadow band's colour **/
    glm::vec4 far;                  /**< where each cascade ends, from the near plane **/
    glm::vec4 radii;                /**< each cascade's radius, which scales its normal bias **/
    glm::vec4 plane;                /**< the camera's near plane, as Cascades::plane **/
    glm::vec4 cascades;             /**< x how many cascades, y the blend **/
};

/**
 * The scene uniform for one frame, with a shadow map of one layer drawn through one matrix. The
 * one cascade reaches every distance, as fitShadowToView() or fitShadow() fits it.
 *
 * @param lightViewProjection the matrix the shadow pass drew through, or the identity with a
 *        strength of zero when there is no shadow
 * @param texel the size of one shadow map texel in uv, which the shadow lookup steps by
 **/
SceneUniforms pack(const Settings& settings, const glm::mat4& lightViewProjection, float texel);

/**
 * The scene uniform for one frame, with a shadow map of a layer for each cascade.
 *
 * @param texel the size of one texel in uv, the same in every layer
 **/
SceneUniforms pack(const Settings& settings, const Cascades& cascades, float texel);

};  // namespace v3d::render::realtime::lighting
