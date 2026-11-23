/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec3.hpp>

namespace v3d::moya::light {

/**
 * A Planckian radiator at a temperature, as a light with a colour temperature emits it: scale is
 * what kernel::spectrum::planck() at that temperature is multiplied by for a luminance of one,
 * and colour is the radiator's ACEScg colour at that luminance. Both are summed at 1 nm through
 * the observer. A temperature of zero or less gives a scale of zero and black.
 **/
struct Planckian {
    float scale { 0.0F };
    glm::vec3 colour { 0.0F };
};

Planckian planckian(float temperature);

};  // namespace v3d::moya::light
