/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembly.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Hierarchy.h>
#include <moya/libmoya/accel/MovingHierarchy.h>
#include <moya/libmoya/accel/Options.h>
#include <moya/libmoya/accel/Root.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/accel/Triangles.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::accel {

namespace {

/**
 * Appends a hierarchy's nodes to an array of its kind of node, and its references to an
 * assembly's, its children moved past what each held. Gives its root.
 **/
template <typename N>
std::uint32_t append(std::vector<N> & nodes, std::vector<std::uint32_t> & references, const std::vector<N> & added,
    const std::vector<std::uint32_t> & addedReferences) {
    if (nodes.size() + added.size() >= device::cpu::kLeaf || references.size() + addedReferences.size() >= device::cpu::kLeaf) {
        throw std::length_error("a scene's hierarchies hold fewer nodes and references than a child can name");
    }
    const auto nodeBase = static_cast<std::uint32_t>(nodes.size());
    const auto referenceBase = static_cast<std::uint32_t>(references.size());
    for (N node : added) {
        for (std::uint32_t slot = 0; slot < device::cpu::kWidth; ++slot) {
            const std::uint32_t child = node.child[slot];
            if (child == device::cpu::kEmpty) {
                continue;
            }
            node.child[slot] = (child & device::cpu::kLeaf) != 0 ? (device::cpu::kLeaf | ((child & ~device::cpu::kLeaf) + referenceBase)) :
                                                                   child + nodeBase;
        }
        nodes.push_back(node);
    }
    references.insert(references.end(), addedReferences.begin(), addedReferences.end());
    return nodeBase;
}

/** The part of bounds over the shutter that covers a part of it. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the part's begin, then its end
LinearBox restrict(const LinearBox & bounds, const geometry::Prototypes & prototypes, float begin, float end) {
    const float span = prototypes.close - prototypes.open;
    if (!(span > 0.0F)) {
        return bounds;
    }
    return LinearBox { .start = bounds.at((begin - prototypes.open) / span), .end = bounds.at((end - prototypes.open) / span) };
}

/** The bounds over a time range of each of a geometry's primitives, by its form. **/
Sweep sweepOf(const geometry::Prototypes & prototypes, const geometry::Geometry & geometry) {
    switch (geometry.form) {
        case kernel::geometry::primitive::Form::Triangles:
            return triangleSweep(geometry.points, prototypes.points, prototypes.times);
        case kernel::geometry::primitive::Form::RoundPoints:
        case kernel::geometry::primitive::Form::FlatPoints:
            return pointSweep(geometry.points, geometry.widths, prototypes.points, prototypes.widths, prototypes.times);
        case kernel::geometry::primitive::Form::Quadric:
            break;
    }
    const kernel::geometry::Sampled held = geometry.quadric;
    return [held, &prototypes](std::uint32_t, float begin, float end) {
        return quadricSweep(held, prototypes.quadrics, prototypes.times, begin, end);
    };
}

/** Builds each geometry's hierarchy into an assembly, and gives its bounds over the shutter. **/
std::vector<LinearBox> assembleGeometries(device::CpuDevice & device, const geometry::Prototypes & prototypes, Assembly & out) {
    std::vector<LinearBox> bounds;
    bounds.reserve(prototypes.geometries.size());
    Options unsplit;
    unsplit.spatial = false;
    for (const geometry::Geometry & geometry : prototypes.geometries) {
        if (geometry.moves()) {
            const MovingHierarchy hierarchy =
                buildMoving(device, geometry.count, sweepOf(prototypes, geometry), prototypes.open, prototypes.close, unsplit);
            bounds.push_back(hierarchy.bounds);
            out.geometries.push_back(Root { .node = append(out.motionNodes, out.references, hierarchy.nodes, hierarchy.references), .moving = true });
            continue;
        }
        Hierarchy hierarchy;
        if (geometry.form == kernel::geometry::primitive::Form::Triangles) {
            const auto first = static_cast<std::ptrdiff_t>(geometry.points.first);
            const std::vector<glm::vec3> points(prototypes.points.begin() + first, prototypes.points.begin() + first + geometry.points.count);
            hierarchy = build(device, triangleBounds(points), triangleClip(points));
        } else {
            const Sweep sweep = sweepOf(prototypes, geometry);
            std::vector<Box> boxes;
            boxes.reserve(geometry.count);
            for (std::uint32_t primitive = 0; primitive < geometry.count; ++primitive) {
                boxes.push_back(sweep(primitive, prototypes.open, prototypes.open).start);
            }
            const Clip clip = [&boxes](std::uint32_t primitive, const Box & within) {
                return boxes[primitive].within(within);
            };
            hierarchy = build(device, boxes, clip, unsplit);
        }
        bounds.push_back(LinearBox::still(hierarchy.bounds));
        out.geometries.push_back(Root { .node = append(out.nodes, out.references, hierarchy.nodes, hierarchy.references), .moving = false });
    }
    return bounds;
}

/** Fills the CPU's record of each instance a group holds, and gives whether the group moves. **/
bool placeMembers(const geometry::Prototypes & prototypes, const geometry::Group & members, Assembly & out) {
    bool moving = false;
    for (const std::uint32_t index : members.instances) {
        const geometry::Instance & instance = prototypes.instances[index];
        const bool shape = instance.kind == geometry::Prototype::Geometry;
        const Root root = shape ? out.geometries[instance.prototype] : out.groups[instance.prototype];
        device::cpu::Instance & placed = out.instances[index];
        placed.toPrototype = glm::inverse(instance.transform);
        placed.root = root.node;
        placed.moving = root.moving ? 1U : 0U;
        placed.geometry = shape ? instance.prototype : device::cpu::kGroup;
        placed.id = instance.id;
        placed.visibility = instance.visibility;
        if (instance.moves()) {
            placed.keys = static_cast<std::uint32_t>(out.keys.size());
            placed.keyCount = static_cast<std::uint32_t>(instance.keys.size());
            out.keys.insert(out.keys.end(), instance.keys.begin(), instance.keys.end());
        }
        moving = moving || root.moving || instance.moves();
    }
    return moving;
}

/**
 * Builds a group's hierarchy into an assembly over its instances, each bounded by its prototype's
 * bounds over the shutter, and gives its root and its own bounds.
 **/
std::pair<Root, LinearBox> assembleGroup(device::CpuDevice & device, const geometry::Prototypes & prototypes, const geometry::Group & members,
    const std::vector<LinearBox> & geometryBounds, const std::vector<LinearBox> & groupBounds, Assembly & out) {
    const auto boundsOf = [&](const geometry::Instance & instance) -> const LinearBox & {
        return instance.kind == geometry::Prototype::Geometry ? geometryBounds[instance.prototype] : groupBounds[instance.prototype];
    };
    Options options;
    options.spatial = false;
    if (!placeMembers(prototypes, members, out)) {
        std::vector<Box> boxes;
        boxes.reserve(members.instances.size());
        for (const std::uint32_t index : members.instances) {
            const geometry::Instance & instance = prototypes.instances[index];
            boxes.push_back(moveBox(boundsOf(instance).start, instance.transform));
        }
        const Clip clip = [&boxes](std::uint32_t primitive, const Box & within) {
            return boxes[primitive].within(within);
        };
        Hierarchy hierarchy = build(device, boxes, clip, options);
        for (std::uint32_t & reference : hierarchy.references) {
            reference = members.instances[reference];
        }
        const Root root { .node = append(out.nodes, out.references, hierarchy.nodes, hierarchy.references), .moving = false };
        return std::pair { root, LinearBox::still(hierarchy.bounds) };
    }
    const Sweep sweep = [&](std::uint32_t primitive, float begin, float end) {
        const geometry::Instance & instance = prototypes.instances[members.instances[primitive]];
        const LinearBox held = restrict(boundsOf(instance), prototypes, begin, end);
        return instance.moves() ? sweepBox(held.swept(), instance.keys, begin, end) : moveBox(held, instance.transform);
    };
    MovingHierarchy hierarchy =
        buildMoving(device, static_cast<std::uint32_t>(members.instances.size()), sweep, prototypes.open, prototypes.close, options);
    for (std::uint32_t & reference : hierarchy.references) {
        reference = members.instances[reference];
    }
    const Root root { .node = append(out.motionNodes, out.references, hierarchy.nodes, hierarchy.references), .moving = true };
    return std::pair { root, hierarchy.bounds };
}

};  // namespace

Assembly assemble(device::CpuDevice & device, const geometry::Prototypes & prototypes) {
    Assembly out;
    out.instances.resize(prototypes.instances.size());
    const std::vector<LinearBox> geometryBounds = assembleGeometries(device, prototypes, out);
    // a group comes after every group it holds, so each member's root and bounds are known
    std::vector<LinearBox> groupBounds;
    groupBounds.reserve(prototypes.groups.size());
    for (const geometry::Group & members : prototypes.groups) {
        const auto [root, bounds] = assembleGroup(device, prototypes, members, geometryBounds, groupBounds, out);
        out.groups.push_back(root);
        groupBounds.push_back(bounds);
    }
    const auto [root, bounds] = assembleGroup(device, prototypes, prototypes.world, geometryBounds, groupBounds, out);
    out.world = root;
    out.bounds = bounds;
    return out;
}

};  // namespace v3d::moya::accel
