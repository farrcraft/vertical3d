/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/bench/Rays.h>

#include <moya/libmoya/kernel/geometry/Ray.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::bench {

std::vector<kernel::geometry::Ray> coherent(const glm::vec3 & low, const glm::vec3 & high, std::uint32_t side) {
    std::vector<kernel::geometry::Ray> out;
    out.reserve(static_cast<std::size_t>(side) * side);
    const glm::vec3 extent = high - low;
    for (std::uint32_t row = 0; row < side; ++row) {
        for (std::uint32_t column = 0; column < side; ++column) {
            kernel::geometry::Ray ray;
            const float x = low.x + (extent.x * (static_cast<float>(column) + 0.5F) / static_cast<float>(side));
            const float y = low.y + (extent.y * (static_cast<float>(row) + 0.5F) / static_cast<float>(side));
            ray.origin = glm::vec3 { x, y, high.z + 1.0F };
            ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
            ray.time = 0.5F;
            out.push_back(ray);
        }
    }
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the box, then how many rays and their seed
std::vector<kernel::geometry::Ray> incoherent(const glm::vec3 & low, const glm::vec3 & high, std::uint32_t count, std::uint32_t seed) {
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> unit(0.0F, 1.0F);
    std::vector<kernel::geometry::Ray> out;
    out.reserve(count);
    // a full turn, in radians
    constexpr float kTurn = 6.28318531F;
    for (std::uint32_t i = 0; i < count; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = low + ((high - low) * glm::vec3 { unit(random), unit(random), unit(random) });
        const float z = (2.0F * unit(random)) - 1.0F;
        const float around = kTurn * unit(random);
        const float across = std::sqrt(std::max(0.0F, 1.0F - (z * z)));
        ray.direction = glm::vec3 { across * std::cos(around), across * std::sin(around), z };
        ray.time = unit(random);
        out.push_back(ray);
    }
    return out;
}

};  // namespace v3d::moya::bench
