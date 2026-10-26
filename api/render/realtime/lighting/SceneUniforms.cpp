/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SceneUniforms.h"

#include "Settings.h"
#include "Shadow.h"

#include <cstddef>
#include <cstdint>
#include <limits>

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
static_assert(offsetof(SceneUniforms, lightViewProjection) == 3 * VEC4, "the light's matrices are at offset 48");
static_assert(offsetof(SceneUniforms, shadow) == offsetof(SceneUniforms, lightViewProjection) + (maximumCascades * sizeof(glm::mat4)),
    "the shadow terms are at offset 304");
static_assert(offsetof(SceneUniforms, colour) == offsetof(SceneUniforms, shadow) + VEC4, "the light's colour is at offset 320");
static_assert(offsetof(SceneUniforms, shadowColour) == offsetof(SceneUniforms, colour) + VEC4, "the shadow's colour is at offset 336");
static_assert(offsetof(SceneUniforms, far) == offsetof(SceneUniforms, shadowColour) + VEC4, "the cascades' far ends are at offset 352");
static_assert(offsetof(SceneUniforms, radii) == offsetof(SceneUniforms, far) + VEC4, "the cascades' radii are at offset 368");
static_assert(offsetof(SceneUniforms, plane) == offsetof(SceneUniforms, radii) + VEC4, "the near plane is at offset 384");
static_assert(offsetof(SceneUniforms, cascades) == offsetof(SceneUniforms, plane) + VEC4, "the cascade terms are at offset 400");
static_assert(sizeof(SceneUniforms) == offsetof(SceneUniforms, cascades) + VEC4, "the Scene block is 416 bytes");

/**
 **/
SceneUniforms pack(const Settings& settings, const glm::mat4& lightViewProjection, float texel) {
    Cascades one;
    one.count = 1;
    one.lights[0] = lightViewProjection;
    // a radius of one leaves the normal bias as it is, and a cascade with no far end and a
    // plane of zero takes every fragment
    one.bounds[0].radius = 1.0F;
    one.far[0] = std::numeric_limits<float>::max();
    return pack(settings, one, texel);
}

/**
 **/
SceneUniforms pack(const Settings& settings, const Cascades& cascades, float texel) {
    SceneUniforms uniforms;
    uniforms.light = glm::vec4(glm::normalize(settings.light), settings.fill);
    uniforms.thresholds = glm::vec4(settings.shadowThreshold, settings.litThreshold, 0.0F, 0.0F);
    uniforms.bands = glm::vec4(settings.bands, 0.0F);
    uniforms.shadow = glm::vec4(texel, settings.shadowStrength, settings.normalBias, 0.0F);
    uniforms.colour = glm::vec4(settings.colour, 0.0F);
    uniforms.shadowColour = glm::vec4(settings.shadowColour, 0.0F);
    uniforms.far = glm::vec4(0.0F);
    uniforms.radii = glm::vec4(1.0F);
    for (uint32_t i = 0; i < maximumCascades; i++) {
        uniforms.lightViewProjection[i] = i < cascades.count ? cascades.lights[i] : glm::mat4(1.0F);
        if (i < cascades.count) {
            uniforms.far[static_cast<int>(i)] = cascades.far[i];
            uniforms.radii[static_cast<int>(i)] = cascades.bounds[i].radius;
        }
    }
    uniforms.plane = cascades.plane;
    uniforms.cascades = glm::vec4(static_cast<float>(cascades.count), cascades.blend, 0.0F, 0.0F);
    return uniforms;
}

};  // namespace v3d::render::realtime::lighting
