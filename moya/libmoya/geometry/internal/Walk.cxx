/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Walk.h>

#include <api/log/Logger.h>
#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry::internal {

Walk::Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger) :
    scene_(scene), logger_(logger), groups_(byGroup(scene)) {
}

void Walk::group(const scene::Handle & group, const glm::mat4 & toWorld, std::vector<std::uint32_t> & chain) {
    const auto members = groups_.find(group.valid() ? group.index : kWorld);
    if (members == groups_.end()) {
        return;
    }
    for (const scene::Handle & instance : members->second) {
        place(instance, toWorld, chain);
    }
}

Flat Walk::take() {
    return std::move(flat_);
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
    flat_.reports.push_back(message);
    if (logger_) {
        logger_->get()->warn("{}", message);
    }
}

void Walk::place(const scene::Handle & instance, const glm::mat4 & parent, std::vector<std::uint32_t> & chain) {
    const scene::Handle prototype = scene_.value(instance, "prototype").handleArray().front();
    if (!prototype.valid() || !scene_.contains(prototype)) {
        return;
    }
    if (chain.size() == kernel::geometry::kInstanceDepth) {
        report("instance " + scene_.name(instance) + " is nested deeper than " + std::to_string(kernel::geometry::kInstanceDepth) +
            " instances, and is left out");
        return;
    }
    const glm::mat4 toWorld = parent * scene_.value(instance, "transform").matrixAt(0);
    const std::string & kind = scene_.schema().kind(prototype.kind).name;
    chain.push_back(instance.index);
    if (kind == "group") {
        if (std::find(open_.begin(), open_.end(), prototype.index) != open_.end()) {
            report("group " + scene_.name(prototype) + " holds itself, and is left out where it does");
        } else {
            open_.push_back(prototype.index);
            group(prototype, toWorld, chain);
            open_.pop_back();
        }
    } else if (kind == "mesh") {
        mesh(prototype, toWorld, chain);
    } else {
        report("a " + kind + " is not yet traced, so " + scene_.name(prototype) + " is left out");
    }
    chain.pop_back();
}

void Walk::mesh(const scene::Handle & handle, const glm::mat4 & toWorld, const std::vector<std::uint32_t> & chain) {
    const Mesh & cut = cutMesh(handle);
    kernel::geometry::Placement placement;
    placement.depth = static_cast<std::uint32_t>(chain.size());
    std::copy(chain.begin(), chain.end(), placement.instances);
    placement.first = static_cast<std::uint32_t>(flat_.faces.size());
    placement.count = static_cast<std::uint32_t>(cut.triangles.faces.size());
    for (const std::uint32_t corner : cut.triangles.corners) {
        const glm::vec3 & point = cut.points[static_cast<std::size_t>(cut.indices[corner])];
        flat_.points.emplace_back(toWorld * glm::vec4(point, 1.0F));
    }
    flat_.faces.insert(flat_.faces.end(), cut.triangles.faces.begin(), cut.triangles.faces.end());
    flat_.runs.insert(flat_.runs.end(), cut.triangles.faces.size(), static_cast<std::uint32_t>(flat_.placements.size()));
    flat_.placements.push_back(placement);
}

const Walk::Mesh & Walk::cutMesh(const scene::Handle & handle) {
    const auto found = meshes_.find(handle.index);
    if (found != meshes_.end()) {
        return found->second;
    }
    Mesh mesh;
    const scene::Value & points = scene_.value(handle, "P");
    mesh.points.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        mesh.points.push_back(points.triple(i));
    }
    mesh.indices = scene_.value(handle, "indices").integerArray();
    mesh.triangles = triangulate(mesh.points, scene_.value(handle, "vertices").integerArray(), mesh.indices);
    if (mesh.triangles.dropped > 0) {
        report("mesh " + scene_.name(handle) + " has " + std::to_string(mesh.triangles.dropped) + " faces that cover no area");
    }
    return meshes_.emplace(handle.index, std::move(mesh)).first->second;
}

};  // namespace v3d::moya::geometry::internal
