/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Listener.h"

#include <cmath>
#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::audio {

namespace {

bool finite(const glm::vec3& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

/**
 * The shortest vector taken as a direction, relative to the length it was made from. Shorter
 * than this, normalising it gives noise.
 **/
const float minimumLength = 1.0e-6F;

};  // namespace

std::optional<Listener> orthonormal(const Listener& listener) noexcept {
    if (!finite(listener.position) || !finite(listener.forward) || !finite(listener.up) || !finite(listener.right)) {
        return std::nullopt;
    }
    if (glm::length(listener.forward) < minimumLength) {
        return std::nullopt;
    }
    const glm::vec3 forward = glm::normalize(listener.forward);
    // what is left of up once its part along forward is taken out. Nothing is left of an up
    // parallel to forward
    const glm::vec3 across = listener.up - (glm::dot(listener.up, forward) * forward);
    const float upLength = glm::length(listener.up);
    if (upLength < minimumLength || glm::length(across) < minimumLength * upLength) {
        return std::nullopt;
    }
    const glm::vec3 up = glm::normalize(across);
    // right is at right angles to both, so only its side is the caller's to give
    const glm::vec3 side = glm::cross(forward, up);
    const float along = glm::dot(listener.right, side);
    const float rightLength = glm::length(listener.right);
    if (rightLength < minimumLength || std::abs(along) < minimumLength * rightLength) {
        return std::nullopt;
    }
    Listener made;
    made.position = listener.position;
    made.forward = forward;
    made.up = up;
    made.right = along > 0.0F ? side : -side;
    return made;
}

bool mirrored(const Listener& listener) noexcept {
    return glm::dot(listener.right, glm::cross(listener.forward, listener.up)) < 0.0F;
}

glm::vec3 relative(const Listener& listener, const glm::vec3& point) noexcept {
    const glm::vec3 offset = point - listener.position;
    return {glm::dot(offset, listener.right), glm::dot(offset, listener.up), -glm::dot(offset, listener.forward)};
}

};  // namespace v3d::audio
