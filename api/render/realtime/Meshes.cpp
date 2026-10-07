/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Meshes.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/vulkan/renderer/Lit.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>

#include <cstdint>
#include <cstring>
#include <optional>

namespace v3d::render::realtime {

namespace {

/**
 * One draw of a part of an entry, with its model matrix and base colour pushed.
 **/
DrawItem item(const MeshRegistry::Entry& entry, const MeshRegistry::Part& part, PipelineHandle pipeline,
    const glm::mat4& model, float outline, uint32_t firstJoint) {
    vulkan::renderer::Lit::Object object;
    object.model = model;
    object.baseColour = part.baseColour;
    object.outline = outline;
    object.firstJoint = firstJoint;

    DrawItem item;
    item.pipeline = pipeline;
    item.material = part.material;
    entry.mesh->describe(&item);
    item.firstIndex = part.firstIndex;
    item.indices = part.indexCount;
    std::memcpy(item.push.data(), &object, sizeof(object));
    item.pushSize = sizeof(object);
    return item;
}

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
 * One draw of every part of an entry.
 **/
void submit(const MeshRegistry::Entry& entry, PipelineHandle pipeline, const glm::mat4& model, float outline,
    uint32_t firstJoint, Pass* pass) {
    for (const MeshRegistry::Part& part : entry.parts) {
        pass->submit(item(entry, part, pipeline, model, outline, firstJoint));
    }
}

};  // namespace

/**
 **/
void meshes(const entt::registry& registry, float alpha, const MeshRegistry& meshes, const vulkan::renderer::Lit& lit,
    float outline, Pass* pass, const Poses& poses) {
    auto view = registry.view<const ecs::component::Transform, const component::Mesh>();
    const bool outlined = outline > 0.0f;

    // two walks rather than one, so every hull is in the pass before any surface
    for (int walk = outlined ? 0 : 1; walk < 2; walk++) {
        const bool surfaces = walk == 1;
        for (const entt::entity entity : view) {
            const MeshRegistry::Entry* entry = meshes.resolve(view.get<const component::Mesh>(entity).mesh);
            if (entry == nullptr) {
                continue;
            }
            const std::optional<uint32_t> first = firstJoint(*entry, entity, poses);
            if (!first) {
                continue;
            }
            const bool skinned = static_cast<bool>(entry->skin);
            const PipelineHandle surface = skinned ? lit.skinnedCel() : lit.cel();
            const PipelineHandle hull = skinned ? lit.skinnedOutline() : lit.outline();
            const glm::mat4 model = ecs::interpolated<ecs::component::Transform>(registry, entity, alpha).matrix();
            submit(*entry, surfaces ? surface : hull, model, surfaces ? 0.0f : outline, *first, pass);
        }
    }
}

/**
 **/
void casters(const entt::registry& registry, float alpha, const MeshRegistry& meshes, const vulkan::renderer::Lit& lit,
    Pass* pass, const Poses& poses) {
    auto view = registry.view<const ecs::component::Transform, const component::Mesh>();
    for (const entt::entity entity : view) {
        const component::Mesh& mesh = view.get<const component::Mesh>(entity);
        if (!mesh.castsShadow) {
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
        const PipelineHandle pipeline = entry->skin ? lit.skinnedShadow() : lit.shadow();
        const glm::mat4 model = ecs::interpolated<ecs::component::Transform>(registry, entity, alpha).matrix();
        submit(*entry, pipeline, model, 0.0f, *first, pass);
    }
}

};  // namespace v3d::render::realtime
