/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "MeshRegistry.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::render::realtime {

/**
 * Every skinned entity's palette for one frame, end to end, and where each starts - ADR-0071.
 *
 * Made by poses(), handed to renderer::Lit::scene() as the frame's palette, and to meshes() and
 * casters() so each skinned draw names its own start. Both have to be given the same Poses, or
 * a draw reads another entity's joints.
 **/
class Poses final {
 public:
    /**
     * @return every matrix, in the order the entities were walked
     **/
    const std::vector<glm::mat4>& palette() const noexcept;

    /**
     * @return where an entity's palette starts, or nothing for one that was not posed
     **/
    std::optional<uint32_t> firstJoint(entt::entity entity) const;

    /**
     * Append an entity's palette. An entity is posed once a frame, so it is not looked for first.
     **/
    void add(entt::entity entity, const std::vector<glm::mat4>& joints);

 private:
    std::vector<glm::mat4> palette_;
    /**
     * Each posed entity and where its palette starts. A list rather than a map, because a frame
     * poses a few dozen entities and a vector moves without allocating, which a Poses returned
     * by value has to.
     **/
    std::vector<std::pair<entt::entity, uint32_t>> starts_;
};

/**
 * Pose every entity carrying an ecs::component::Transform and a component::Mesh whose entry has
 * a skin - ADR-0070.
 *
 * An entity with an ecs::component::Playback is sampled from it, drawn alpha of the way from its
 * previous step (ADR-0060), and blended out of the clip it is fading from. One with none, or
 * playing nothing, stands in its rest pose. An entity whose handle was released is skipped.
 *
 * Called once a frame, before Lit::scene(), while the frame is built.
 *
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one
 **/
Poses poses(const entt::registry& registry, float alpha, const MeshRegistry& meshes);

};  // namespace v3d::render::realtime
