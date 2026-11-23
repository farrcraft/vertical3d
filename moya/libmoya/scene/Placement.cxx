/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Placement.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::scene {

namespace {

/** How nearly parallel the direction may be to y before x is taken as up. **/
constexpr float kParallel = 0.999F;

};  // namespace

glm::mat4 aimed(const glm::vec3 & from, const glm::vec3 & to) {
    glm::mat4 placed(1.0F);
    placed[3] = glm::vec4(from, 1.0F);
    const glm::vec3 towards = to - from;
    if (glm::length(towards) == 0.0F) {
        return placed;
    }
    const glm::vec3 back = -glm::normalize(towards);
    const glm::vec3 up = std::abs(back.y) > kParallel ? glm::vec3(1.0F, 0.0F, 0.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
    const glm::vec3 right = glm::normalize(glm::cross(up, back));
    placed[0] = glm::vec4(right, 0.0F);
    placed[1] = glm::vec4(glm::cross(back, right), 0.0F);
    placed[2] = glm::vec4(back, 0.0F);
    return placed;
}

};  // namespace v3d::moya::scene
