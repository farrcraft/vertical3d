/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shadow.h"

#include <api/render/realtime/component/Mesh.h>

#include <api/ecs/component/Transform.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/geometric.hpp>

namespace v3d::render::realtime::shadow {

/**
 **/
glm::mat4 light(const glm::vec3& towards, const glm::vec3& centre, float radius) {
    // the light looks along +z of its own basis, as a camera does, from the light towards the
    // centre
    const glm::vec3 z = -glm::normalize(towards);
    const glm::vec3 eye = centre - z * (radius * 2.0f);

    // an up parallel to the view has no right to cross into, which a light straight overhead
    // would give
    const glm::vec3 up = std::abs(z.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 x = glm::normalize(glm::cross(up, z));
    const glm::vec3 y = glm::cross(z, x);

    // the basis transposed, then the eye moved to the origin
    glm::mat4 view(1.0f);
    view[0][0] = x.x;
    view[1][0] = x.y;
    view[2][0] = x.z;
    view[0][1] = y.x;
    view[1][1] = y.y;
    view[2][1] = y.z;
    view[0][2] = z.x;
    view[1][2] = z.y;
    view[2][2] = z.z;
    view[3][0] = -glm::dot(x, eye);
    view[3][1] = -glm::dot(y, eye);
    view[3][2] = -glm::dot(z, eye);

    // a square box a radius either side, y flipped into Vulkan's clip space, and depth from
    // zero at the eye to one four radii along
    glm::mat4 projection(1.0f);
    projection[0][0] = 1.0f / radius;
    projection[1][1] = -1.0f / radius;
    projection[2][2] = 1.0f / (radius * 4.0f);

    return projection * view;
}

/**
 **/
std::optional<Bounds> fit(const entt::registry& registry, std::span<const glm::vec3> alsoCover, float margin) {
    std::vector<glm::vec3> points;
    auto view = registry.view<const ecs::component::Transform, const component::Mesh>();
    for (const entt::entity entity : view) {
        if (view.get<const component::Mesh>(entity).castsShadow) {
            points.push_back(view.get<const ecs::component::Transform>(entity).position);
        }
    }
    if (points.empty()) {
        return std::nullopt;
    }
    points.insert(points.end(), alsoCover.begin(), alsoCover.end());

    Bounds bounds;
    for (const glm::vec3& point : points) {
        bounds.centre += point;
    }
    bounds.centre /= static_cast<float>(points.size());

    float spread = 0.0f;
    for (const glm::vec3& point : points) {
        spread = std::max(spread, glm::length(point - bounds.centre));
    }
    bounds.radius = spread + margin;
    return bounds;
}

};  // namespace v3d::render::realtime::shadow
