/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Dicing.h>

#include <moya/libmoya/kernel/camera/Camera.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry {

namespace {

/** The nearest depth a perspective camera's pixel is measured at, so that it never vanishes. **/
constexpr float kNearest = 1.0e-6F;

};  // namespace

Dicing::Dicing(const kernel::camera::Camera & camera, float edgeLength, const glm::mat4 & placement) :
    toCamera_(glm::inverse(camera.toWorld) * placement), perspective_(camera.perspective), edgeLength_(edgeLength) {
    const float across = std::fabs(camera.screen.y - camera.screen.x) / camera.resolution.x;
    const float down = std::fabs(camera.screen.w - camera.screen.z) / camera.resolution.y;
    pixel_ = std::min(across, down) * (perspective_ ? camera.tanHalfFov : 1.0F);
}

std::uint32_t Dicing::operator()(const std::vector<glm::vec3> & along) const {
    if (!(edgeLength_ > 0.0F)) {
        return kMostPieces;
    }
    float pixels = 0.0F;
    for (std::size_t at = 1; at < along.size(); ++at) {
        const glm::vec3 from { toCamera_ * glm::vec4(along[at - 1], 1.0F) };
        const glm::vec3 to { toCamera_ * glm::vec4(along[at], 1.0F) };
        const float depth = perspective_ ? std::max(std::fabs((from.z + to.z) * 0.5F), kNearest) : 1.0F;
        pixels += glm::length(to - from) / (pixel_ * depth);
    }
    const float pieces = std::ceil(pixels / edgeLength_);
    if (!(pieces < static_cast<float>(kMostPieces))) {
        return kMostPieces;
    }
    return std::max(static_cast<std::uint32_t>(pieces), 1U);
}

};  // namespace v3d::moya::geometry
