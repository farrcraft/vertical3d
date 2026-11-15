/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/Names.h>

#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::integrate::internal {

namespace {

/** The scene's objects of a kind, by their index in the scene, and none where none is. **/
std::vector<std::optional<scene::Handle>> byIndex(const scene::Scene & scene, const std::string & kind) {
    std::vector<std::optional<scene::Handle>> out;
    for (const scene::Handle & handle : scene.objects(kind)) {
        if (handle.index >= out.size()) {
            out.resize(handle.index + 1U);
        }
        out[handle.index] = handle;
    }
    return out;
}

};  // namespace

std::vector<std::string> namesOf(output::Cryptomatte::Names names, const scene::Scene & scene, const geometry::Prototypes & prototypes) {
    std::vector<std::string> out;
    if (names == output::Cryptomatte::Names::Material) {
        for (const scene::Handle & handle : prototypes.materialHandles) {
            out.push_back(scene.contains(handle) ? scene.name(handle) : std::string());
        }
        return out;
    }
    const std::vector<std::optional<scene::Handle>> instances = byIndex(scene, "instance");
    out.resize(instances.size());
    for (std::size_t i = 0; i < instances.size(); ++i) {
        out[i] = instances[i] ? scene.name(*instances[i]) : std::string();
    }
    if (names == output::Cryptomatte::Names::Asset) {
        const std::vector<std::optional<scene::Handle>> groups = byIndex(scene, "group");
        for (const geometry::Instance & instance : prototypes.instances) {
            if (instance.kind != geometry::Prototype::Group || instance.id >= out.size()) {
                continue;
            }
            const std::uint32_t group = prototypes.groups[instance.prototype].id;
            if (group < groups.size() && groups[group]) {
                out[instance.id] = scene.name(*groups[group]);
            }
        }
    }
    return out;
}

};  // namespace v3d::moya::integrate::internal
