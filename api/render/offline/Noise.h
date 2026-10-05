/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * Perlin's improved noise, remapped to SL's range: in [0, 1], at 0.5 on every lattice point,
 * and continuous with a continuous gradient between them.
 *
 * Its permutation is shuffled once by a `type::Random` of a fixed seed rather than taken
 * from Perlin's table. A shader's pattern is then the same on every machine, every standard
 * library and every run, so a reference image can pin it.
 **/
float noise(const glm::vec3 & point);

};  // namespace v3d::render::offline
