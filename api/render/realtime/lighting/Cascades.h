/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/render/realtime/lighting/ShadowBounds.h>

#include <array>
#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::lighting {

/**
 * The most cascades a shadow map has. SceneUniforms holds this many.
 **/
inline constexpr uint32_t maximumCascades = 4;

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

};  // namespace v3d::render::realtime::lighting
