/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Walk.h>

#include <api/log/Logger.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::geometry::internal {

Walk::Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger) :
    scene_(scene), logger_(logger), groups_(byGroup(scene)) {
}

Prototypes Walk::take() {
    prototypes_.world = members(scene::Handle {});
    deepen(prototypes_.world, 0);
    return std::move(prototypes_);
}

std::map<std::uint32_t, std::vector<scene::Handle>> Walk::byGroup(const scene::Scene & scene) {
    std::map<std::uint32_t, std::vector<scene::Handle>> out;
    for (const scene::Handle & instance : scene.objects("instance")) {
        const scene::Handle group = scene.value(instance, "group").handleArray().front();
        if (!group.valid()) {
            out[kWorld].push_back(instance);
        } else if (scene.contains(group)) {
            out[group.index].push_back(instance);
        }
    }
    return out;
}

void Walk::report(const std::string & message) {
    if (!reported_.insert(message).second) {
        return;
    }
    prototypes_.reports.push_back(message);
    if (logger_) {
        logger_->get()->warn("{}", message);
    }
}

Group Walk::members(const scene::Handle & group) {
    Group out;
    const auto found = groups_.find(group.valid() ? group.index : kWorld);
    if (found == groups_.end()) {
        return out;
    }
    for (const scene::Handle & handle : found->second) {
        const std::optional<std::uint32_t> at = instance(handle, group);
        if (at) {
            out.instances.push_back(*at);
        }
    }
    return out;
}

std::optional<std::uint32_t> Walk::instance(const scene::Handle & handle, const scene::Handle & holder) {
    const scene::Handle prototype = scene_.value(handle, "prototype").handleArray().front();
    if (!prototype.valid() || !scene_.contains(prototype)) {
        return std::nullopt;
    }
    Instance out;
    out.id = handle.index;
    out.transform = scene_.value(handle, "transform").matrixAt(0);
    const std::pair<const char *, std::uint32_t> kinds[] = { { "camera", kernel::geometry::kCameraRay },
        { "shadow", kernel::geometry::kShadowRay }, { "indirect", kernel::geometry::kIndirectRay } };
    for (const auto & [name, bit] : kinds) {
        if (scene_.value(handle, name).integerArray().front() != 0) {
            out.visibility |= bit;
        }
    }
    const std::string & kind = scene_.schema().kind(prototype.kind).name;
    if (kind == "group") {
        // the world is held by nothing, so only a group's member can close a loop
        if (holder.valid() && held(prototype.index).contains(holder.index)) {
            report("group " + scene_.name(prototype) + " holds itself, and is left out where it does");
            return std::nullopt;
        }
        out.kind = Prototype::Group;
        out.prototype = group(prototype);
    } else if (kind == "mesh") {
        out.kind = Prototype::Mesh;
        out.prototype = mesh(prototype);
    } else {
        report("a " + kind + " is not yet traced, so " + scene_.name(prototype) + " is left out");
        return std::nullopt;
    }
    const auto index = static_cast<std::uint32_t>(prototypes_.instances.size());
    prototypes_.instances.push_back(out);
    handles_.push_back(handle);
    return index;
}

std::uint32_t Walk::group(const scene::Handle & handle) {
    const auto found = groupIndex_.find(handle.index);
    if (found != groupIndex_.end()) {
        return found->second;
    }
    // its members first, so that a group comes after every group it holds. No member closes a
    // loop back to this group, so this ends
    Group gathered = members(handle);
    const auto index = static_cast<std::uint32_t>(prototypes_.groups.size());
    prototypes_.groups.push_back(std::move(gathered));
    groupIndex_.emplace(handle.index, index);
    return index;
}

std::uint32_t Walk::mesh(const scene::Handle & handle) {
    const auto found = meshIndex_.find(handle.index);
    if (found != meshIndex_.end()) {
        return found->second;
    }
    std::vector<glm::vec3> points;
    const scene::Value & values = scene_.value(handle, "P");
    points.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        points.push_back(values.triple(i));
    }
    const std::vector<std::int32_t> & indices = scene_.value(handle, "indices").integerArray();
    const Triangles triangles = triangulate(points, scene_.value(handle, "vertices").integerArray(), indices);
    if (triangles.dropped > 0) {
        report("mesh " + scene_.name(handle) + " has " + std::to_string(triangles.dropped) + " faces that cover no area");
    }
    Mesh mesh;
    mesh.first = static_cast<std::uint32_t>(prototypes_.faces.size());
    mesh.count = static_cast<std::uint32_t>(triangles.faces.size());
    for (const std::uint32_t corner : triangles.corners) {
        prototypes_.points.push_back(points[static_cast<std::size_t>(indices[corner])]);
    }
    prototypes_.faces.insert(prototypes_.faces.end(), triangles.faces.begin(), triangles.faces.end());
    const auto index = static_cast<std::uint32_t>(prototypes_.meshes.size());
    prototypes_.meshes.push_back(mesh);
    meshIndex_.emplace(handle.index, index);
    return index;
}

const std::set<std::uint32_t> & Walk::held(std::uint32_t group) {
    const auto found = held_.find(group);
    if (found != held_.end()) {
        return found->second;
    }
    std::set<std::uint32_t> out;
    std::vector<std::uint32_t> open { group };
    while (!open.empty()) {
        const std::uint32_t at = open.back();
        open.pop_back();
        const auto members = groups_.find(at);
        if (members == groups_.end()) {
            continue;
        }
        for (const scene::Handle & handle : members->second) {
            const scene::Handle prototype = scene_.value(handle, "prototype").handleArray().front();
            if (prototype.valid() && scene_.contains(prototype) && scene_.schema().kind(prototype.kind).name == "group" &&
                out.insert(prototype.index).second) {
                open.push_back(prototype.index);
            }
        }
    }
    return held_.emplace(group, std::move(out)).first->second;
}

void Walk::deepen(const Group & group, std::uint32_t depth) {
    for (const std::uint32_t index : group.instances) {
        if (depth == kernel::geometry::kInstanceDepth) {
            report("instance " + scene_.name(handles_[index]) + " is nested deeper than " + std::to_string(kernel::geometry::kInstanceDepth) +
                " instances, and is left out");
            continue;
        }
        const Instance & instance = prototypes_.instances[index];
        // a group met again at one depth reports nothing new
        if (instance.kind == Prototype::Group && deepened_.emplace(instance.prototype, depth + 1).second) {
            deepen(prototypes_.groups[instance.prototype], depth + 1);
        }
    }
}

};  // namespace v3d::moya::geometry::internal
