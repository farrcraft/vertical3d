/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Poses.h"

#include <api/render/realtime/component/Mesh.h>
#include <api/type/animation/Clock.h>
#include <api/type/animation/Pose.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Playback.h>
#include <api/ecs/component/Transform.h>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace v3d::render::realtime {

namespace {

/**
 * One of a skin's clips sampled over its rest pose, or the rest pose when the clip is none or
 * not one the skin has.
 **/
type::animation::Pose sampled(const MeshRegistry::Skin& skin, uint32_t clip, float time, float duration, bool loops) {
    type::animation::Pose pose = type::animation::rest(skin.skeleton);
    if (clip < skin.clips.size()) {
        const type::animation::Clock clock(duration, loops);
        type::animation::sample(skin.clips[clip], clock.sample(time), &pose);
    }
    return pose;
}

/**
 * The pose a playback stands in, faded out of the clip it is leaving.
 **/
type::animation::Pose posed(const MeshRegistry::Skin& skin, const ecs::component::Playback& playback) {
    type::animation::Pose current = sampled(skin, playback.clip, playback.time, playback.duration, playback.loops);
    if (playback.from == ecs::component::Playback::none || playback.fade >= 1.0f) {
        return current;
    }
    const type::animation::Pose leaving =
        sampled(skin, playback.from, playback.fromTime, playback.fromDuration, playback.fromLoops);
    return type::animation::blend(leaving, current, playback.fade);
}

};  // namespace

/**
 **/
const std::vector<glm::mat4>& Poses::palette() const noexcept {
    return palette_;
}

/**
 **/
std::optional<uint32_t> Poses::firstJoint(entt::entity entity) const {
    for (const std::pair<entt::entity, uint32_t>& start : starts_) {
        if (start.first == entity) {
            return start.second;
        }
    }
    return std::nullopt;
}

/**
 **/
void Poses::add(entt::entity entity, const std::vector<glm::mat4>& joints) {
    starts_.emplace_back(entity, static_cast<uint32_t>(palette_.size()));
    palette_.insert(palette_.end(), joints.begin(), joints.end());
}

/**
 **/
Poses poses(const entt::registry& registry, float alpha, const MeshRegistry& meshes) {
    Poses result;
    std::vector<glm::mat4> joints;
    auto view = registry.view<const ecs::component::Transform, const component::Mesh>();
    for (const entt::entity entity : view) {
        const MeshRegistry::Entry* entry = meshes.resolve(view.get<const component::Mesh>(entity).mesh);
        if (entry == nullptr || !entry->skin) {
            continue;
        }
        const MeshRegistry::Skin& skin = *entry->skin;
        const type::animation::Pose pose = registry.all_of<ecs::component::Playback>(entity)
            ? posed(skin, ecs::interpolated<ecs::component::Playback>(registry, entity, alpha))
            : type::animation::rest(skin.skeleton);
        type::animation::palette(skin.skeleton, pose, &joints);
        result.add(entity, joints);
    }
    return result;
}

};  // namespace v3d::render::realtime
