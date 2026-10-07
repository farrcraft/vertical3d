/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "LitSettings.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

/**
 * Set 2, binding 0, laid out as shaders/lit/lit.glsl declares the Scene block. std140 puts
 * every member here at its natural offset, so the struct maps straight across.
 **/
struct SceneUniforms final {
    glm::vec4 light;                /**< xyz towards the key light, normalised; w the fill **/
    glm::vec4 thresholds;           /**< x shadow to mid, y mid to lit **/
    glm::vec4 bands;                /**< xyz the shadow, mid and lit multipliers **/
    glm::mat4 lightViewProjection;  /**< what the shadow map was drawn through **/
    glm::vec4 shadow;               /**< x a shadow map texel in uv, y strength, z normal bias **/
    glm::vec4 colour;               /**< xyz the light's colour, over the mid and lit bands **/
    glm::vec4 shadowColour;         /**< xyz the shadow band's colour **/
};

/**
 * The scene uniform for one frame.
 *
 * @param lightViewProjection the matrix the shadow pass drew through, or the identity with a
 *        strength of zero when there is no shadow
 * @param texel the size of one shadow map texel in uv, which the shadow lookup steps by
 **/
SceneUniforms pack(const LitSettings& settings, const glm::mat4& lightViewProjection, float texel);

};  // namespace v3d::render::realtime
