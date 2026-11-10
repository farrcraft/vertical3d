/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/**
 * What the scene's options set for the integrator. A path ends after maxDepth bounces in all, or
 * when a bounce of one kind would take that kind past its own limit. From rouletteDepth bounces
 * on, Russian roulette may end it. A path that leaves the scene meets the environment, an ACEScg
 * radiance.
 **/
struct Settings {
    std::uint32_t maxDepth { 12 };
    std::uint32_t diffuseDepth { 4 };
    std::uint32_t glossyDepth { 4 };
    std::uint32_t specularDepth { 12 };
    std::uint32_t transmissionDepth { 12 };
    std::uint32_t rouletteDepth { 3 };
    glm::vec3 environment { 0.0F };
};

};  // namespace v3d::moya::kernel::integrate
