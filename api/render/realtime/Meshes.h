/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "MeshRegistry.h"
#include "Pass.h"
#include "Poses.h"

#include <entt/entt.hpp>

namespace v3d::render::realtime {

namespace vulkan::renderer {
class Lit;
};  // namespace vulkan::renderer

/**
 * Every entity carrying both an ecs::component::Transform and a component::Mesh, submitted to a
 * lit pass - ADR-0063.
 *
 * Each is drawn where its transform puts it, alpha of the way from its previous step when it
 * has one (ADR-0060), a draw per part of its registry entry with that part's albedo and base
 * colour. A skinned entry is drawn with Lit's skinned pipelines, in the pose poses() gave it;
 * one the poses do not name is skipped, since it has no palette to be drawn with. Outlines go in first,
 * for every entity, and surfaces after, so the pass draws each hull before any surface that
 * might cover it. An entity whose handle has been released is skipped.
 *
 * The pass is the caller's, and has to name the scene Lit::scene() returned for this frame.
 *
 * @param alpha Engine::alpha(), the fraction of a step elapsed since the last one
 * @param outline how far the outline hull is pushed out - LitSettings::outline. Zero submits no
 *        outlines
 * @param poses this frame's, whose palette the scene the pass names was written with
 **/
void meshes(const entt::registry& registry, float alpha, const MeshRegistry& meshes, const vulkan::renderer::Lit& lit,
    float outline, Pass* pass, const Poses& poses = Poses());

/**
 * Every entity carrying both an ecs::component::Transform and a component::Mesh that casts a
 * shadow, submitted to a shadow pass with Lit::shadow(). Placed and posed as meshes() places
 * and poses it, so a shadow follows what casts it between steps.
 *
 * The pass is the caller's. It draws into a target with sampled depth and no colour, and names
 * the scene Lit::scene() returned for this frame and a depth bias. The lit pass names the
 * target in Pass::reads(), so the frame records this one first - ADR-0068.
 **/
void casters(const entt::registry& registry, float alpha, const MeshRegistry& meshes, const vulkan::renderer::Lit& lit,
    Pass* pass, const Poses& poses = Poses());

};  // namespace v3d::render::realtime
