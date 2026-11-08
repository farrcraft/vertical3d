/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/World.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/scene/Scene.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry {

namespace {

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

/**
 * Places a mesh's triangles as they stand at a time, at the end of a chain of instances, inside
 * its transformation.
 **/
void placeMesh(const Prototypes & prototypes, float time, const Mesh & mesh, const glm::mat4 & toWorld, kernel::geometry::Placement placement,
    Flat & out) {
    placement.first = static_cast<std::uint32_t>(out.faces.size());
    placement.count = mesh.count;
    for (std::uint32_t corner = 0; corner < mesh.points.count; ++corner) {
        const glm::vec3 point = kernel::geometry::pointAt(mesh.points, prototypes.points.data(), prototypes.times.data(), corner, time);
        out.points.emplace_back(toWorld * glm::vec4(point, 1.0F));
    }
    out.faces.insert(out.faces.end(), prototypes.faces.begin() + mesh.first, prototypes.faces.begin() + mesh.first + mesh.count);
    out.runs.insert(out.runs.end(), mesh.count, static_cast<std::uint32_t>(out.placements.size()));
    out.placements.push_back(placement);
}

/**
 * Places every instance in a group as it stands at a time, after the chain that reached the group
 * and inside its transformation.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the group, then the chain that reached it
void placeGroup(const Prototypes & prototypes, float time, const Group & group, const glm::mat4 & toWorld, const kernel::geometry::Placement & chain,
    Flat & out) {
    if (chain.depth == kernel::geometry::kInstanceDepth) {
        return;
    }
    for (const std::uint32_t index : group.instances) {
        const Instance & instance = prototypes.instances[index];
        kernel::geometry::Placement placement = chain;
        placement.instances[placement.depth++] = instance.id;
        placement.visibility &= instance.visibility;
        const glm::mat4 inner = toWorld * transformAt(instance, time);
        if (instance.kind == Prototype::Group) {
            placeGroup(prototypes, time, prototypes.groups[instance.prototype], inner, placement, out);
        } else {
            placeMesh(prototypes, time, prototypes.meshes[instance.prototype], inner, placement, out);
        }
    }
}

};  // namespace

float heldTime(const Prototypes & prototypes, float time) {
    return std::fmax(prototypes.open, std::fmin(time, prototypes.close));
}

glm::mat4 transformAt(const Instance & instance, float time) {
    return instance.moves() ? kernel::geometry::transformAt(instance.keys.data(), static_cast<std::uint32_t>(instance.keys.size()), time) :
                              instance.transform;
}

Flat flatten(const Prototypes & prototypes, float time) {
    Flat out;
    placeGroup(prototypes, heldTime(prototypes, time), prototypes.world, glm::mat4(1.0F), kernel::geometry::Placement {}, out);
    out.reports = prototypes.reports;
    return out;
}

Flat flatten(const Prototypes & prototypes) {
    return flatten(prototypes, prototypes.open);
}

Flat flatten(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger) {
    return flatten(gather(scene, logger));
}

World::World(device::Device & device, const Flat & flat) :
    points_(uploaded(device, flat.points)), faces_(uploaded(device, flat.faces)), placements_(uploaded(device, flat.placements)),
    runs_(uploaded(device, flat.runs)) {
}

kernel::geometry::Placed World::view() const {
    return kernel::geometry::Placed { .points = device::view<glm::vec3>(*points_), .faces = device::view<std::uint32_t>(*faces_),
        .placements = device::view<kernel::geometry::Placement>(*placements_) };
}

kernel::Array<std::uint32_t> World::runs() const {
    return device::view<std::uint32_t>(*runs_);
}

std::uint32_t World::triangles() const {
    return view().faces.size;
}

};  // namespace v3d::moya::geometry
