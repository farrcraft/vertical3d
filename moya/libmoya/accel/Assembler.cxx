/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembler.h>

#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/accel/internal/Part.h>
#include <moya/libmoya/accel/internal/Parts.h>
#include <moya/libmoya/accel/Root.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/scene/Change.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::accel {

Assembler::Assembler(device::CpuDevice & device, const boost::shared_ptr<log::Logger> & logger) : device_(device), logger_(logger) {
}

void Assembler::update(const scene::Scene & scene) {
    // the scene's index of every object that changed, whose cut geometry is made again
    std::set<std::uint32_t> changed;
    if (built_) {
        for (const scene::Change & change : scene.changes(version_)) {
            changed.insert(change.handle.index);
            cache_.forget(change.handle.index);
        }
    }
    prototypes_ = geometry::gather(scene, logger_, cache_);
    builtGeometries_.clear();
    builtGroups_.clear();

    Assembly out;
    out.instances.resize(prototypes_.instances.size());
    std::map<std::uint32_t, internal::Part> geometryParts;
    std::vector<bool> geometryBuilt;
    std::vector<LinearBox> geometryBounds;
    for (const geometry::Geometry & geometry : prototypes_.geometries) {
        const auto held = geometryParts_.find(geometry.id);
        const bool build = held == geometryParts_.end() || cache_.made().contains(geometry.id);
        const internal::Part part = build ? internal::geometryPart(device_, prototypes_, geometry) : held->second;
        if (build) {
            builtGeometries_.push_back(geometry.id);
        }
        geometryBuilt.push_back(build);
        geometryBounds.push_back(part.bounds);
        out.geometries.push_back(internal::appendPart(out, part, nullptr));
        geometryParts.insert_or_assign(geometry.id, part);
    }

    // a group comes after every group it holds, so each member's root, bounds and whether it was
    // built are known
    std::map<std::uint32_t, Held> groupParts;
    std::vector<bool> groupBuilt;
    std::vector<LinearBox> groupBounds;
    const auto assembleGroup = [&](const geometry::Group & group) {
        std::vector<std::uint32_t> members;
        bool stale = changed.contains(group.id);
        for (const std::uint32_t index : group.instances) {
            const geometry::Instance & instance = prototypes_.instances[index];
            members.push_back(instance.id);
            const bool rebuilt = instance.kind == geometry::Prototype::Geometry ? geometryBuilt[instance.prototype] : groupBuilt[instance.prototype];
            stale = stale || changed.contains(instance.id) || rebuilt;
        }
        const bool moving = internal::placeMembers(prototypes_, group, out);
        const auto held = groupParts_.find(group.id);
        stale = stale || held == groupParts_.end() || held->second.members != members || held->second.part.moving != moving;
        Held kept { .part = stale ? internal::groupPart(device_, prototypes_, group, moving, geometryBounds, groupBounds) : held->second.part,
            .members = members };
        if (stale) {
            builtGroups_.push_back(group.id);
        }
        const Root root = internal::appendPart(out, kept.part, &group);
        const LinearBox bounds = kept.part.bounds;
        groupParts.insert_or_assign(group.id, std::move(kept));
        return std::pair { root, std::pair { bounds, stale } };
    };
    for (const geometry::Group & group : prototypes_.groups) {
        const auto [root, held] = assembleGroup(group);
        out.groups.push_back(root);
        groupBounds.push_back(held.first);
        groupBuilt.push_back(held.second);
    }
    const auto [root, held] = assembleGroup(prototypes_.world);
    out.world = root;
    out.bounds = held.first;

    // what no prototype placed this time is dropped
    geometryParts_ = std::move(geometryParts);
    groupParts_ = std::move(groupParts);
    assembly_ = std::move(out);
    version_ = scene.version();
    built_ = true;
}

};  // namespace v3d::moya::accel
