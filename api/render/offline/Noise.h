/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * Perlin's improved noise, moved into SL's range: in [0, 1], at 0.5 on every lattice point,
 * and continuous with a continuous gradient between them.
 *
 * Its permutation is shuffled once by a `type::Random` of a fixed seed rather than taken
 * from Perlin's table, so a shader's pattern is the same on every machine, every standard
 * library and every run, which is what lets a reference pin one.
 **/
float noise(const glm::vec3 & point);

};  // namespace v3d::render::offline
