/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SceneUniforms.h"

#include "Settings.h"

#include <cstddef>

#include <glm/geometric.hpp>

namespace v3d::render::realtime::lighting {

namespace {

constexpr std::size_t VEC4 = sizeof(glm::vec4);

};  // namespace

// the offsets lit.glsl's std140 Scene block puts each member at, which nothing at runtime checks.
// Every member is a vec4 or a mat4, which std140 packs with no padding between them
static_assert(offsetof(SceneUniforms, light) == 0, "light is at offset 0");
static_assert(offsetof(SceneUniforms, thresholds) == VEC4, "thresholds are at offset 16");
static_assert(offsetof(SceneUniforms, bands) == 2 * VEC4, "bands are at offset 32");
static_assert(offsetof(SceneUniforms, lightViewProjection) == 3 * VEC4, "the light's matrix is at offset 48");
static_assert(offsetof(SceneUniforms, shadow) == offsetof(SceneUniforms, lightViewProjection) + sizeof(glm::mat4),
    "the shadow terms are at offset 112");
static_assert(offsetof(SceneUniforms, colour) == offsetof(SceneUniforms, shadow) + VEC4, "the light's colour is at offset 128");
static_assert(offsetof(SceneUniforms, shadowColour) == offsetof(SceneUniforms, colour) + VEC4, "the shadow's colour is at offset 144");
static_assert(sizeof(SceneUniforms) == offsetof(SceneUniforms, shadowColour) + VEC4, "the Scene block is 160 bytes");

/**
 **/
SceneUniforms pack(const Settings& settings, const glm::mat4& lightViewProjection, float texel) {
    SceneUniforms uniforms;
    uniforms.light = glm::vec4(glm::normalize(settings.light), settings.fill);
    uniforms.thresholds = glm::vec4(settings.shadowThreshold, settings.litThreshold, 0.0F, 0.0F);
    uniforms.bands = glm::vec4(settings.bands, 0.0F);
    uniforms.lightViewProjection = lightViewProjection;
    uniforms.shadow = glm::vec4(texel, settings.shadowStrength, settings.normalBias, 0.0F);
    uniforms.colour = glm::vec4(settings.colour, 0.0F);
    uniforms.shadowColour = glm::vec4(settings.shadowColour, 0.0F);
    return uniforms;
}

};  // namespace v3d::render::realtime::lighting
