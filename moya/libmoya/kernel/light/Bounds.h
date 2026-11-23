/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/**
 * What a node of the light tree bounds, after Conty Estevez and Kulla: a box that holds its
 * lights over the shutter, and a cone that holds the directions they emit along.
 *
 * - lower and upper are the box's corners.
 * - axis is the cone's unit axis. Each light's emitting face points within the angle whose cosine
 *   is cosSpread of it, and the light emits within the angle whose cosine is cosFalloff past that.
 *   A cosSpread of -1 takes in every direction.
 * - power is the sum of the lights' power, and zero for bounds that hold nothing.
 **/
struct Bounds {
    glm::vec3 lower { 0.0F };
    glm::vec3 upper { 0.0F };
    glm::vec3 axis { 0.0F, 0.0F, 1.0F };
    float cosSpread { -1.0F };
    float cosFalloff { 0.0F };
    float power { 0.0F };
};

};  // namespace v3d::moya::kernel::light
