/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/** The depth limits until the options set them: Cycles'. **/
inline constexpr std::uint32_t kMaxDepth = 12;
inline constexpr std::uint32_t kKindDepth = 4;
inline constexpr std::uint32_t kRouletteDepth = 3;

/**
 * How a path finds the lights with a shape: by multiple importance sampling of a light and the
 * BSDF at each bounce, by sampling a light alone, or by the BSDF alone.
 **/
enum class Strategy : std::uint8_t {
    Mis,
    Light,
    Bsdf
};

/**
 * What the scene's options set for the integrator. A path ends after maxDepth bounces in all, or
 * when a bounce of one kind would take that kind past its own limit. From rouletteDepth bounces
 * on, Russian roulette may end it. A path that leaves the scene meets the environment, an ACEScg
 * radiance. The strategy says how it finds the lights.
 **/
struct Settings {
    std::uint32_t maxDepth { kMaxDepth };
    std::uint32_t diffuseDepth { kKindDepth };
    std::uint32_t glossyDepth { kKindDepth };
    std::uint32_t specularDepth { kMaxDepth };
    std::uint32_t transmissionDepth { kMaxDepth };
    std::uint32_t rouletteDepth { kRouletteDepth };
    glm::vec3 environment { 0.0F };
    Strategy strategy { Strategy::Mis };
};

};  // namespace v3d::moya::kernel::integrate
