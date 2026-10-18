/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Meshes.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/vulkan/renderer/Lit.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>

#include "Poses.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace v3d::render::realtime::entity {

namespace {

/**
 * An entity about to be drawn: its entry, where it stands, and where its palette starts.
 **/
struct Placed final {
    const MeshRegistry::Entry* entry;
    glm::mat4 model;
    uint32_t firstJoint;
};

/**
 * Where an entity's palette starts: zero for a static entry, which reads none, and nothing for
 * a skinned one the poses do not name.
 **/
std::optional<uint32_t> firstJoint(const MeshRegistry::Entry& entry, entt::entity entity, const Poses& poses) {
    if (!entry.skin) {
        return 0;
    }
    return poses.firstJoint(entity);
}

/**
 * Every entity carrying a transform and a mesh, grouped by the entry it draws. Groups keep the
 * order their first entities were met in, and so do the entities in a group. The draws
 * submitted therefore do not depend on how a container hashes. An entity whose entry does not
 * resolve, or a skinned one the poses do not name, is left out.
 *
 * @param castersOnly true to leave out an entity whose mesh casts no shadow
 **/
std::vector<std::vector<Placed>> gather(const entt::registry& registry, float alpha, const MeshRegistry& meshes,
    const Poses& poses, bool castersOnly) {
    std::vector<std::vector<Placed>> groups;
    std::unordered_map<const MeshRegistry::Entry*, std::size_t> found;
    auto view = registry.view<const ecs::component::Transform, const component::Mesh>();
    for (const entt::entity entity : view) {
        const component::Mesh& mesh = view.get<const component::Mesh>(entity);
        if (castersOnly && !mesh.castsShadow) {
            continue;
        }
        const MeshRegistry::Entry* entry = meshes.resolve(mesh.mesh);
        if (entry == nullptr) {
            continue;
        }
        const std::optional<uint32_t> first = firstJoint(*entry, entity, poses);
        if (!first) {
            continue;
        }
        const auto [at, added] = found.try_emplace(entry, groups.size());
        if (added) {
            groups.emplace_back();
        }
        const glm::mat4 model = ecs::interpolated<ecs::component::Transform>(registry, entity, alpha).matrix();
        groups[at->second].push_back(Placed{entry, model, *first});
    }
    return groups;
}

/**
 * One draw of each part of a group's entry, with an instance per entity. Each instance's object
 * is added to the scene in turn, so a draw's objects lie side by side from its first instance.
 **/
void submit(const std::vector<Placed>& group, PipelineHandle pipeline, float outline, vulkan::renderer::Lit* lit,
    Pass* pass) {
    const MeshRegistry::Entry& entry = *group.front().entry;
    for (const MeshRegistry::Part& part : entry.parts) {
        DrawItem item;
        item.pipeline = pipeline;
        item.material = part.material;
        entry.mesh->describe(&item);
        item.firstIndex = part.firstIndex;
        item.indices = part.indexCount;
        item.instances = static_cast<uint32_t>(group.size());
        for (std::size_t instance = 0; instance < group.size(); instance++) {
            vulkan::renderer::Lit::Object object;
            object.model = group[instance].model;
            object.baseColour = part.baseColour;
            object.outline = outline;
            object.firstJoint = group[instance].firstJoint;
            const uint32_t index = lit->object(object);
            if (instance == 0) {
                item.firstInstance = index;
            }
        }
        pass->submit(item);
    }
}

};  // namespace

/**
 **/
void meshes(const entt::registry& registry, float alpha, const MeshRegistry& meshes, vulkan::renderer::Lit* lit,
    float outline, Pass* pass, const Poses& poses) {
    const std::vector<std::vector<Placed>> groups = gather(registry, alpha, meshes, poses, false);
    const bool outlined = outline > 0.0F;

    // two loops over the groups, so every hull is submitted before any surface
    for (int walk = outlined ? 0 : 1; walk < 2; walk++) {
        const bool surfaces = walk == 1;
        for (const std::vector<Placed>& group : groups) {
            const bool skinned = static_cast<bool>(group.front().entry->skin);
            const PipelineHandle surface = skinned ? lit->skinnedCel() : lit->cel();
            const PipelineHandle hull = skinned ? lit->skinnedOutline() : lit->outline();
            submit(group, surfaces ? surface : hull, surfaces ? 0.0F : outline, lit, pass);
        }
    }
}

/**
 **/
void casters(const entt::registry& registry, float alpha, const MeshRegistry& meshes, vulkan::renderer::Lit* lit,
    Pass* pass, const Poses& poses) {
    for (const std::vector<Placed>& group : gather(registry, alpha, meshes, poses, true)) {
        const PipelineHandle pipeline = group.front().entry->skin ? lit->skinnedShadow() : lit->shadow();
        submit(group, pipeline, 0.0F, lit, pass);
    }
}

};  // namespace v3d::render::realtime::entity
