/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sprites.h"

#include <api/render/realtime/component/Sprite.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>

#include <glm/geometric.hpp>

namespace v3d::render::realtime {

/**
 **/
void sprites(const entt::registry& registry, float alpha, const glm::vec3& right,
    const glm::vec3& up, const glm::vec3& depthAxis, DepthOrder* order) {
    auto view = registry.view<const ecs::component::Transform, const component::Sprite>();
    for (const entt::entity entity : view) {
        const component::Sprite& sprite = view.get<const component::Sprite>(entity);
        const ecs::component::Transform transform =
            ecs::interpolated<ecs::component::Transform>(registry, entity, alpha);
        const glm::vec3& feet = transform.position;
        const glm::vec3 across = right * (sprite.size.x * transform.scale.x * 0.5f);
        const glm::vec3 rise = up * (sprite.size.y * transform.scale.y);

        // around the perimeter from the top-left, so uv0 lands on corners[0]
        const WorldCanvas::Corners corners = {
            feet - across + rise,
            feet + across + rise,
            feet + across,
            feet - across
        };
        order->quad(glm::dot(feet, depthAxis), corners, sprite.uv0, sprite.uv1, sprite.tint, sprite.texture);
    }
}

};  // namespace v3d::render::realtime
