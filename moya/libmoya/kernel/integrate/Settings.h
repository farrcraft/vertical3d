/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/**
 * The depth limits until the options set them: Cycles'. The volume's is the diffuse one's, since
 * Cycles' of zero lets a path scatter once in a medium, and moya's counts that scattering.
 **/
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
 * How a light sample draws its light: by the light tree, by what each light may bring to the
 * shading point, or in proportion to each light's power alone.
 **/
enum class Picking : std::uint8_t {
    Tree,
    Power
};

/**
 * What the scene's options set for the integrator. A path ends after maxDepth bounces in all, or
 * when a bounce of one kind would take that kind past its own limit. A scattering event in a
 * medium is a bounce of its own kind, which volumeDepth limits. From rouletteDepth bounces
 * on, Russian roulette may end it. The strategy says how it finds the lights, and picking how a
 * light sample draws one.
 **/
struct Settings {
    std::uint32_t maxDepth { kMaxDepth };
    std::uint32_t diffuseDepth { kKindDepth };
    std::uint32_t glossyDepth { kKindDepth };
    std::uint32_t specularDepth { kMaxDepth };
    std::uint32_t transmissionDepth { kMaxDepth };
    std::uint32_t volumeDepth { kKindDepth };
    std::uint32_t rouletteDepth { kRouletteDepth };
    Strategy strategy { Strategy::Mis };
    Picking picking { Picking::Tree };
};

};  // namespace v3d::moya::kernel::integrate
