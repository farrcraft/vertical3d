/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shadow.h"

#include <api/render/realtime/component/Mesh.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::lighting {

namespace {

/**
 * The light's own axes: its right, its up, and the direction it looks along, which is +z of its
 * basis as a camera's is.
 **/
struct Basis final {
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 along;
};

Basis basis(const glm::vec3& towards) {
    Basis out;
    out.along = -glm::normalize(towards);
    // an up parallel to the view has no right to cross into, which a light straight overhead
    // would give
    const glm::vec3 up = std::abs(out.along.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    out.right = glm::normalize(glm::cross(up, out.along));
    out.up = glm::cross(out.along, out.right);
    return out;
}

bool finite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

};  // namespace

/**
 **/
glm::mat4 shadowLight(const glm::vec3& towards, const glm::vec3& centre, float radius, type::camera::Profile::Hand hand) {
    // the light looks from the light towards the centre
    const Basis axes = basis(towards);
    const glm::vec3 z = axes.along;
    const glm::vec3 eye = centre - z * (radius * 2.0f);
    const glm::vec3 y = axes.up;
    // the mirrored hand negates the right and keeps the up, as Camera::createView negates view x
    const glm::vec3 x = hand == type::camera::Profile::Hand::DirectionCrossUp ? -axes.right : axes.right;

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
std::optional<ShadowBounds> fitShadow(const entt::registry& registry, std::span<const glm::vec3> alsoCover, float margin, float alpha) {
    std::vector<glm::vec3> points;
    auto view = registry.view<const ecs::component::Transform, const component::Mesh>();
    for (const entt::entity entity : view) {
        if (view.get<const component::Mesh>(entity).castsShadow) {
            points.push_back(ecs::interpolated<ecs::component::Transform>(registry, entity, alpha).position);
        }
    }
    if (points.empty()) {
        return std::nullopt;
    }
    points.insert(points.end(), alsoCover.begin(), alsoCover.end());

    ShadowBounds bounds;
    for (const glm::vec3& point : points) {
        bounds.centre += point;
    }
    bounds.centre /= static_cast<float>(points.size());

    float spread = 0.0f;
    for (const glm::vec3& point : points) {
        spread = std::max(spread, glm::length(point - bounds.centre));
    }
    bounds.radius = std::max(spread + margin, minimumShadowRadius);
    return bounds;
}

/**
 **/
std::optional<ShadowBounds> fitShadowToView(const glm::mat4& viewProjection, float distance, const glm::vec3& towards, uint32_t texels) {
    // a NaN fails every comparison, so each test is written to fail it
    if (!(distance > 0.0f) || !std::isfinite(distance) || !finite(towards) || !(glm::length(towards) > 0.0f) || texels < 4) {
        return std::nullopt;
    }
    const float determinant = glm::determinant(viewProjection);
    if (!(std::abs(determinant) > 0.0f) || !std::isfinite(determinant)) {
        return std::nullopt;
    }
    const glm::mat4 inverse = glm::inverse(viewProjection);

    // each edge of the view volume runs from a near corner to the far corner behind it, and is
    // cut at the distance
    std::array<glm::vec3, 8> corners;
    std::size_t next = 0;
    for (const float x : {-1.0f, 1.0f}) {
        for (const float y : {-1.0f, 1.0f}) {
            const glm::vec4 nearClip = inverse * glm::vec4(x, y, 0.0f, 1.0f);
            const glm::vec4 farClip = inverse * glm::vec4(x, y, 1.0f, 1.0f);
            const glm::vec3 nearCorner = glm::vec3(nearClip) / nearClip.w;
            const glm::vec3 farCorner = glm::vec3(farClip) / farClip.w;
            const glm::vec3 edge = farCorner - nearCorner;
            const float length = glm::length(edge);
            corners[next++] = nearCorner;
            corners[next++] = length > distance ? nearCorner + edge * (distance / length) : farCorner;
        }
    }

    ShadowBounds bounds;
    for (const glm::vec3& corner : corners) {
        bounds.centre += corner;
    }
    bounds.centre /= static_cast<float>(corners.size());
    float spread = 0.0f;
    for (const glm::vec3& corner : corners) {
        spread = std::max(spread, glm::length(corner - bounds.centre));
    }
    if (!finite(bounds.centre) || !std::isfinite(spread)) {
        return std::nullopt;
    }

    // the snap below moves the centre up to a texel along each of the map's two axes, so the
    // radius grows by the diagonal of a texel. A texel is two radii over the map's width, which
    // makes the radius spread / (1 - 2 * sqrt(2) / texels)
    const float slack = 2.0f * std::sqrt(2.0f) / static_cast<float>(texels);
    bounds.radius = std::max(spread / (1.0f - slack), minimumShadowRadius);

    const Basis axes = basis(towards);
    const float texel = 2.0f * bounds.radius / static_cast<float>(texels);
    const float across = glm::dot(bounds.centre, axes.right);
    const float up = glm::dot(bounds.centre, axes.up);
    bounds.centre += axes.right * ((std::floor(across / texel) * texel) - across) + axes.up * ((std::floor(up / texel) * texel) - up);
    return bounds;
}

};  // namespace v3d::render::realtime::lighting
