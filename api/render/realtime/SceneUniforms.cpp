/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SceneUniforms.h"

#include <cstddef>

#include <glm/geometric.hpp>

namespace v3d::render::realtime {

// the offsets lit.glsl's std140 Scene block puts each member at, which nothing at runtime checks
static_assert(offsetof(SceneUniforms, light) == 0, "light is at offset 0");
static_assert(offsetof(SceneUniforms, thresholds) == 16, "thresholds are at offset 16");
static_assert(offsetof(SceneUniforms, bands) == 32, "bands are at offset 32");
static_assert(offsetof(SceneUniforms, lightViewProjection) == 48, "the light's matrix is at offset 48");
static_assert(offsetof(SceneUniforms, shadow) == 112, "the shadow terms are at offset 112");
static_assert(offsetof(SceneUniforms, colour) == 128, "the light's colour is at offset 128");
static_assert(offsetof(SceneUniforms, shadowColour) == 144, "the shadow's colour is at offset 144");
static_assert(sizeof(SceneUniforms) == 160, "the Scene block is 160 bytes");

/**
 **/
SceneUniforms pack(const LitSettings& settings, const glm::mat4& lightViewProjection, float texel) {
    SceneUniforms uniforms;
    uniforms.light = glm::vec4(glm::normalize(settings.light), settings.fill);
    uniforms.thresholds = glm::vec4(settings.shadowThreshold, settings.litThreshold, 0.0f, 0.0f);
    uniforms.bands = glm::vec4(settings.bands, 0.0f);
    uniforms.lightViewProjection = lightViewProjection;
    uniforms.shadow = glm::vec4(texel, settings.shadowStrength, settings.normalBias, 0.0f);
    uniforms.colour = glm::vec4(settings.colour, 0.0f);
    uniforms.shadowColour = glm::vec4(settings.shadowColour, 0.0f);
    return uniforms;
}

};  // namespace v3d::render::realtime
