/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembly.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Triangles.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Prototypes.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::accel {

namespace {

/** Appends a hierarchy to an assembly, its children moved past what it held. Gives its root. **/
std::uint32_t append(Assembly & out, const Hierarchy & hierarchy) {
    if (out.nodes.size() + hierarchy.nodes.size() >= device::cpu::kLeaf ||
        out.references.size() + hierarchy.references.size() >= device::cpu::kLeaf) {
        throw std::length_error("a scene's hierarchies hold fewer nodes and references than a child can name");
    }
    const auto nodeBase = static_cast<std::uint32_t>(out.nodes.size());
    const auto referenceBase = static_cast<std::uint32_t>(out.references.size());
    for (device::cpu::Node node : hierarchy.nodes) {
        for (std::uint32_t slot = 0; slot < device::cpu::kWidth; ++slot) {
            const std::uint32_t child = node.child[slot];
            if (child == device::cpu::kEmpty) {
                continue;
            }
            node.child[slot] = (child & device::cpu::kLeaf) != 0 ? (device::cpu::kLeaf | ((child & ~device::cpu::kLeaf) + referenceBase)) :
                                                                   child + nodeBase;
        }
        out.nodes.push_back(node);
    }
    out.references.insert(out.references.end(), hierarchy.references.begin(), hierarchy.references.end());
    return nodeBase;
}

/** A box moved by a transformation: the box of its eight corners moved. Empty for an empty box. **/
Box transformed(const Box & box, const glm::mat4 & transform) {
    Box out;
    if (box.empty()) {
        return out;
    }
    for (int corner = 0; corner < 8; ++corner) {
        const glm::vec3 point { (corner & 1) != 0 ? box.upper.x : box.lower.x, (corner & 2) != 0 ? box.upper.y : box.lower.y,
            (corner & 4) != 0 ? box.upper.z : box.lower.z };
        out.grow(glm::vec3(transform * glm::vec4(point, 1.0F)));
    }
    return out;
}

};  // namespace

Assembly assemble(device::CpuDevice & device, const geometry::Prototypes & prototypes) {
    Assembly out;
    out.instances.resize(prototypes.instances.size());
    std::vector<Box> meshBounds;
    meshBounds.reserve(prototypes.meshes.size());
    for (const geometry::Mesh & mesh : prototypes.meshes) {
        const auto first = static_cast<std::ptrdiff_t>(mesh.first) * 3;
        const std::vector<glm::vec3> points(prototypes.points.begin() + first, prototypes.points.begin() + first + (std::ptrdiff_t { mesh.count } * 3));
        const Hierarchy hierarchy = build(device, triangleBounds(points), triangleClip(points));
        meshBounds.push_back(hierarchy.bounds);
        out.meshes.push_back(append(out, hierarchy));
    }

    // a group comes after every group it holds, so each member's root and bounds are known
    std::vector<Box> groupBounds;
    groupBounds.reserve(prototypes.groups.size());
    const auto group = [&](const geometry::Group & members) {
        std::vector<Box> boxes;
        boxes.reserve(members.instances.size());
        for (const std::uint32_t index : members.instances) {
            const geometry::Instance & instance = prototypes.instances[index];
            device::cpu::Instance & placed = out.instances[index];
            placed.toPrototype = glm::inverse(instance.transform);
            placed.id = instance.id;
            placed.visibility = instance.visibility;
            if (instance.kind == geometry::Prototype::Mesh) {
                placed.root = out.meshes[instance.prototype];
                placed.first = prototypes.meshes[instance.prototype].first;
                boxes.push_back(transformed(meshBounds[instance.prototype], instance.transform));
            } else {
                placed.root = out.groups[instance.prototype];
                placed.first = device::cpu::kGroup;
                boxes.push_back(transformed(groupBounds[instance.prototype], instance.transform));
            }
        }
        const Clip clip = [&boxes](std::uint32_t primitive, const Box & within) {
            return boxes[primitive].within(within);
        };
        Hierarchy hierarchy = build(device, boxes, clip, Options { .spatial = false });
        for (std::uint32_t & reference : hierarchy.references) {
            reference = members.instances[reference];
        }
        const std::uint32_t root = append(out, hierarchy);
        return std::pair { root, hierarchy.bounds };
    };
    for (const geometry::Group & members : prototypes.groups) {
        const auto [root, bounds] = group(members);
        out.groups.push_back(root);
        groupBounds.push_back(bounds);
    }
    const auto [root, bounds] = group(prototypes.world);
    out.world = root;
    out.bounds = bounds;
    return out;
}

};  // namespace v3d::moya::accel
