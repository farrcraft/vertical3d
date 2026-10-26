/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/lighting/Shadow.h>

#include "Poses.h"

#include <cstdint>
#include <optional>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::realtime::vulkan::renderer {
class Lit;
};  // namespace v3d::render::realtime::vulkan::renderer

namespace v3d::render::realtime::entity {

/**
 * Every entity carrying both an ecs::component::Transform and a component::Mesh, submitted to a
 * lit pass.
 *
 * Each is drawn where its transform puts it, alpha of the way from its previous step when it
 * has one. It takes a draw per part of its registry entry, using that part's albedo and base
 * colour. A skinned entry is drawn with Lit's skinned pipelines, in the pose poses() gave it.
 * One the poses do not name is skipped, since it has no palette to be drawn with. Outlines go
 * in first, for every entity, and surfaces after, so the pass draws each hull before any
 * surface that might cover it. An entity whose handle has been released is skipped.
 *
 * The pass is the caller's, and has to name the scene Lit::scene() returned for this frame.
 * Each draw's object is added to that scene, so scene() is called first.
 *
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one
 * @param outline how far the outline hull is pushed out, from lighting::Settings::outline.
 *        Zero submits no outlines
 * @param poses this frame's, whose palette the scene the pass names was written with
 **/
void meshes(const entt::registry& registry, float alpha, const MeshRegistry& meshes, vulkan::renderer::Lit* lit,
    float outline, Pass* pass, const Poses& poses = Poses());

/**
 * Which cascade a shadow pass draws, and what it covers.
 **/
struct Casting final {
    /** the cascade, which is the pass's layer and the matrix it is drawn through **/
    uint32_t cascade = 0;
    /**
     * The sphere the cascade covers, from lighting::Cascades::bounds, and the direction towards
     * the light. A static caster that cannot throw a shadow into it is left out. Nothing draws
     * every caster, as a map of one layer fitted to the casters needs
     **/
    std::optional<lighting::ShadowBounds> covers;
    glm::vec3 towards{0.0F, 1.0F, 0.0F};
};

/**
 * Every entity carrying both an ecs::component::Transform and a component::Mesh that casts a
 * shadow, submitted to a shadow pass with Lit::shadow(). Placed and posed as meshes() places
 * and poses it, so a shadow follows what casts it between steps.
 *
 * The pass is the caller's. It draws into a target with sampled depth and no colour, into the
 * layer of the cascade it draws, and names the scene Lit::scene() returned for this frame and a
 * depth bias. scene() is called first, as for meshes(). The lit pass names the target in
 * Pass::reads(), so the frame records this one first. A shadow map of several cascades takes a
 * pass and a call for each.
 *
 * A static caster is culled against the cascade by a sphere around its model, scaled by its
 * transform. A skinned one is never culled, since its pose can reach past its bind pose.
 **/
void casters(const entt::registry& registry, float alpha, const MeshRegistry& meshes, vulkan::renderer::Lit* lit,
    Pass* pass, const Poses& poses = Poses(), const Casting& casting = Casting());

};  // namespace v3d::render::realtime::entity
