/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Emitter.h"

#include "Transform.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace v3d::ecs::component {

Emitter::Emitter(uint64_t seed) :
    state(seed) {
}

void emit(entt::registry& registry, float step) {
    for (auto [entity, emitter] : registry.view<Emitter>().each()) {
        const Transform* transform = registry.try_get<Transform>(entity);
        const glm::vec3 origin = transform != nullptr ? transform->position : glm::vec3(0.0f);
        const glm::quat orientation = transform != nullptr ? transform->rotation : glm::identity<glm::quat>();
        type::effect::step(emitter.description, &emitter.state, origin, orientation, step);
    }
}

};  // namespace v3d::ecs::component
