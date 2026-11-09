/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembly.h>

#include <moya/libmoya/accel/internal/Part.h>
#include <moya/libmoya/accel/internal/Parts.h>
#include <moya/libmoya/accel/Root.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Prototypes.h>

#include <utility>
#include <vector>

namespace v3d::moya::accel {

Assembly assemble(device::CpuDevice & device, const geometry::Prototypes & prototypes) {
    Assembly out;
    out.instances.resize(prototypes.instances.size());
    std::vector<LinearBox> geometryBounds;
    geometryBounds.reserve(prototypes.geometries.size());
    for (const geometry::Geometry & geometry : prototypes.geometries) {
        const internal::Part part = internal::geometryPart(device, prototypes, geometry);
        geometryBounds.push_back(part.bounds);
        out.geometries.push_back(internal::appendPart(out, part, nullptr));
    }
    // a group comes after every group it holds, so each member's root and bounds are known
    std::vector<LinearBox> groupBounds;
    groupBounds.reserve(prototypes.groups.size());
    const auto assembleGroup = [&](const geometry::Group & members) {
        const bool moving = internal::placeMembers(prototypes, members, out);
        const internal::Part part = internal::groupPart(device, prototypes, members, moving, geometryBounds, groupBounds);
        return std::pair { internal::appendPart(out, part, &members), part.bounds };
    };
    for (const geometry::Group & members : prototypes.groups) {
        const auto [root, bounds] = assembleGroup(members);
        out.groups.push_back(root);
        groupBounds.push_back(bounds);
    }
    const auto [root, bounds] = assembleGroup(prototypes.world);
    out.world = root;
    out.bounds = bounds;
    return out;
}

};  // namespace v3d::moya::accel
