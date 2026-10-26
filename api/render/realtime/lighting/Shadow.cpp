/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shadow.h"

#include <api/render/realtime/component/Mesh.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>
#include <api/type/camera/Profile.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::lighting {

namespace {

// how far the light's projection reaches along its view, in radii of the bounds it covers
constexpr float DEPTH_RADII = 4.0F;

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
    const glm::vec3 up = std::abs(out.along.y) > 0.99F ? glm::vec3(0.0F, 0.0F, 1.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
    out.right = glm::normalize(glm::cross(up, out.along));
    out.up = glm::cross(out.along, out.right);
    return out;
}

bool finite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

/**
 * The camera's view volume: the four edges from a near corner to the far corner behind it, and
 * the view direction from the middle of the near plane.
 **/
struct Volume final {
    std::array<glm::vec3, 4> near;
    std::array<glm::vec3, 4> far;
    glm::vec3 centre;  /**< the middle of the near plane **/
    glm::vec3 axis;    /**< the view direction, normalised **/
};

/**
 * The view volume of an invertible view projection, or nothing when one of its corners is not
 * finite.
 **/
std::optional<Volume> volume(const glm::mat4& inverse) {
    Volume out;
    std::size_t next = 0;
    glm::vec3 farCentre(0.0F);
    out.centre = glm::vec3(0.0F);
    for (const float x : {-1.0F, 1.0F}) {
        for (const float y : {-1.0F, 1.0F}) {
            const glm::vec4 nearClip = inverse * glm::vec4(x, y, 0.0F, 1.0F);
            const glm::vec4 farClip = inverse * glm::vec4(x, y, 1.0F, 1.0F);
            out.near[next] = glm::vec3(nearClip) / nearClip.w;
            out.far[next] = glm::vec3(farClip) / farClip.w;
            out.centre += out.near[next];
            farCentre += out.far[next];
            if (!finite(out.near[next]) || !finite(out.far[next])) {
                return std::nullopt;
            }
            next++;
        }
    }
    const float corners = static_cast<float>(out.near.size());
    out.centre /= corners;
    farCentre /= corners;
    const glm::vec3 along = farCentre - out.centre;
    if (!(glm::length(along) > 0.0F)) {
        return std::nullopt;
    }
    out.axis = glm::normalize(along);
    return out;
}

/**
 * The inverse of a view projection, or nothing when it has none.
 **/
std::optional<glm::mat4> inverted(const glm::mat4& viewProjection) {
    const float determinant = glm::determinant(viewProjection);
    if (!(std::abs(determinant) > 0.0F) || !std::isfinite(determinant)) {
        return std::nullopt;
    }
    return glm::inverse(viewProjection);
}

/**
 * A sphere around a set of points, with its centre moved across the light to a whole number of
 * the map's texels.
 *
 * The centre is the mean of the points and the radius the farthest of them from it. A set
 * that keeps its shape as the camera turns therefore keeps its radius. The radius has room for
 * the move to a texel.
 **/
std::optional<ShadowBounds> snapped(std::span<const glm::vec3> points, const glm::vec3& towards, uint32_t texels) {
    ShadowBounds bounds;
    for (const glm::vec3& point : points) {
        bounds.centre += point;
    }
    bounds.centre /= static_cast<float>(points.size());
    float spread = 0.0F;
    for (const glm::vec3& point : points) {
        spread = std::max(spread, glm::length(point - bounds.centre));
    }
    if (!finite(bounds.centre) || !std::isfinite(spread)) {
        return std::nullopt;
    }

    // the snap below moves the centre up to a texel along each of the map's two axes, so the
    // radius grows by the diagonal of a texel. A texel is two radii over the map's width, which
    // makes the radius spread / (1 - 2 * sqrt(2) / texels)
    const float slack = 2.0F * std::sqrt(2.0F) / static_cast<float>(texels);
    bounds.radius = std::max(spread / (1.0F - slack), minimumShadowRadius);

    const Basis axes = basis(towards);
    const float texel = 2.0F * bounds.radius / static_cast<float>(texels);
    const float across = glm::dot(bounds.centre, axes.right);
    const float up = glm::dot(bounds.centre, axes.up);
    bounds.centre += axes.right * ((std::floor(across / texel) * texel) - across) + axes.up * ((std::floor(up / texel) * texel) - up);
    return bounds;
}

/**
 * Whether a direction towards the light can place one: finite and not zero.
 **/
bool aims(const glm::vec3& towards) {
    return finite(towards) && glm::length(towards) > 0.0F;
}

};  // namespace

/**
 **/
glm::mat4 shadowLight(const glm::vec3& towards, const ShadowBounds& bounds, type::camera::Profile::Hand hand) {
    const glm::vec3& centre = bounds.centre;
    const float radius = bounds.radius;
    // the light looks from the light towards the centre
    const Basis axes = basis(towards);
    const glm::vec3 z = axes.along;
    const glm::vec3 eye = centre - z * (radius * 2.0F);
    const glm::vec3 y = axes.up;
    // the mirrored hand negates the right and keeps the up, as Camera::createView negates view x
    const glm::vec3 x = hand == type::camera::Profile::Hand::DirectionCrossUp ? -axes.right : axes.right;

    // the basis transposed, then the eye moved to the origin
    glm::mat4 view(1.0F);
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
    glm::mat4 projection(1.0F);
    projection[0][0] = 1.0F / radius;
    projection[1][1] = -1.0F / radius;
    projection[2][2] = 1.0F / (radius * DEPTH_RADII);

    return projection * view;
}

/**
 **/
std::optional<ShadowBounds> fitShadow(const entt::registry& registry, std::span<const glm::vec3> alsoCover, const ShadowFit& fit) {
    const float margin = fit.margin;
    const float alpha = fit.alpha;
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

    float spread = 0.0F;
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
    if (!(distance > 0.0F) || !std::isfinite(distance) || !aims(towards) || texels < 4) {
        return std::nullopt;
    }
    const std::optional<glm::mat4> inverse = inverted(viewProjection);
    if (!inverse) {
        return std::nullopt;
    }

    // each edge of the view volume runs from a near corner to the far corner behind it, and is
    // cut at the distance
    std::array<glm::vec3, 8> corners;
    std::size_t next = 0;
    for (const float x : {-1.0F, 1.0F}) {
        for (const float y : {-1.0F, 1.0F}) {
            const glm::vec4 nearClip = *inverse * glm::vec4(x, y, 0.0F, 1.0F);
            const glm::vec4 farClip = *inverse * glm::vec4(x, y, 1.0F, 1.0F);
            const glm::vec3 nearCorner = glm::vec3(nearClip) / nearClip.w;
            const glm::vec3 farCorner = glm::vec3(farClip) / farClip.w;
            const glm::vec3 edge = farCorner - nearCorner;
            const float length = glm::length(edge);
            corners[next++] = nearCorner;
            corners[next++] = length > distance ? nearCorner + edge * (distance / length) : farCorner;
        }
    }
    return snapped(corners, towards, texels);
}

/**
 **/
std::vector<float> splitView(float near, float distance, uint32_t count, float lambda) {
    if (!(near > 0.0F) || !std::isfinite(near) || !(distance > 0.0F) || !std::isfinite(distance) || count == 0 ||
        count > maximumCascades || std::isnan(lambda)) {
        return {};
    }
    const float blend = std::clamp(lambda, 0.0F, 1.0F);
    const float far = near + distance;
    std::vector<float> splits;
    for (uint32_t i = 1; i <= count; i++) {
        const float part = static_cast<float>(i) / static_cast<float>(count);
        // the eye's distance at the split, each scheme measuring from the eye
        const float logarithmic = near * std::pow(far / near, part);
        const float uniform = near + (distance * part);
        splits.push_back((blend * logarithmic) + ((1.0F - blend) * uniform) - near);
    }
    // exactly the distance, which rounding in the last power could miss
    splits.back() = distance;
    return splits;
}

/**
 **/
std::optional<Cascades> fitCascades(const glm::mat4& viewProjection, std::span<const float> splits, const glm::vec3& towards,
    const CascadeFit& fit) {
    if (splits.empty() || splits.size() > maximumCascades || !aims(towards) || fit.texels < 4 || std::isnan(fit.blend)) {
        return std::nullopt;
    }
    float last = 0.0F;
    for (const float split : splits) {
        if (!(split > last) || !std::isfinite(split)) {
            return std::nullopt;
        }
        last = split;
    }
    const std::optional<glm::mat4> inverse = inverted(viewProjection);
    if (!inverse) {
        return std::nullopt;
    }
    const std::optional<Volume> view = volume(*inverse);
    if (!view) {
        return std::nullopt;
    }

    Cascades cascades;
    cascades.count = static_cast<uint32_t>(splits.size());
    cascades.blend = std::clamp(fit.blend, 0.0F, 0.5F);
    cascades.plane = glm::vec4(view->axis, -glm::dot(view->axis, view->centre));
    for (std::size_t i = 0; i < splits.size(); i++) {
        // the cascade starts inside the one before by the blend, so both cover the band where a
        // fragment is shaded from both
        const float from = i == 0 ? 0.0F : splits[i - 1] * (1.0F - cascades.blend);
        const float to = splits[i];
        std::array<glm::vec3, 8> corners;
        for (std::size_t edge = 0; edge < 4; edge++) {
            // the near corners lie on the near plane, so a point's distance from it grows
            // evenly along an edge, from zero to the far corner's
            const glm::vec3 along = view->far[edge] - view->near[edge];
            const float reach = glm::dot(along, view->axis);
            if (!(reach > 0.0F)) {
                return std::nullopt;
            }
            corners[edge * 2] = view->near[edge] + along * std::min(from / reach, 1.0F);
            corners[(edge * 2) + 1] = view->near[edge] + along * std::min(to / reach, 1.0F);
        }
        const std::optional<ShadowBounds> bounds = snapped(corners, towards, fit.texels);
        if (!bounds) {
            return std::nullopt;
        }
        cascades.bounds[i] = *bounds;
        cascades.lights[i] = shadowLight(towards, *bounds, fit.hand);
        cascades.far[i] = to;
    }
    return cascades;
}

/**
 **/
bool casts(const ShadowBounds& cascade, const glm::vec3& towards, const ShadowBounds& caster) {
    // the box shadowLight() draws through: a radius either side across the light, and from two
    // radii towards the light to two radii away from it
    const Basis axes = basis(towards);
    const glm::vec3 offset = caster.centre - cascade.centre;
    const float across = cascade.radius + caster.radius;
    const float deep = (2.0F * cascade.radius) + caster.radius;
    return std::abs(glm::dot(offset, axes.right)) <= across && std::abs(glm::dot(offset, axes.up)) <= across &&
        std::abs(glm::dot(offset, axes.along)) <= deep;
}

};  // namespace v3d::render::realtime::lighting
