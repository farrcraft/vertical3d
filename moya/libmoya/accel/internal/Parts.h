/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/accel/internal/Part.h>
#include <moya/libmoya/accel/Root.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Prototypes.h>

#include <cstdint>
#include <vector>

/** The steps of assemble(), which accel::Assembler takes too, keeping each prototype's part. **/
namespace v3d::moya::accel::internal {

/**
 * A geometry's hierarchy, as assemble() builds it. A geometry that is cut out is built with no
 * spatial splits, so that each of its primitives lies in one leaf and a ray meets it once.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the geometry, then whether it is cut out
Part geometryPart(device::CpuDevice & device, const geometry::Prototypes & prototypes, const geometry::Geometry & geometry, bool cutOut);

/**
 * Whether each geometry is placed by a chain of instances whose material is cut out. A chain's
 * material is the one its innermost instance that names a material names.
 **/
std::vector<bool> cutOutGeometries(const geometry::Prototypes & prototypes);

/**
 * Fills the CPU's record of each instance a group holds, from the roots the assembly holds for
 * their prototypes and whether the material each names is cut out, and gives whether the group
 * moves.
 **/
bool placeMembers(const geometry::Prototypes & prototypes, const geometry::Group & members, Assembly & out);

/**
 * A group's hierarchy over its instances, as assemble() builds it, each bounded by its
 * prototype's bounds over the shutter. A group that moves is built over the shutter.
 **/
Part groupPart(device::CpuDevice & device, const geometry::Prototypes & prototypes, const geometry::Group & members, bool moving,
    const std::vector<LinearBox> & geometryBounds, const std::vector<LinearBox> & groupBounds);

/**
 * Appends a part to an assembly, and gives its root. A group's part names its members by their
 * instances' indices there.
 *
 * @throws std::length_error when the assembly would hold more nodes or references than a child
 *         can name
 **/
Root appendPart(Assembly & out, const Part & part, const geometry::Group * members);

};  // namespace v3d::moya::accel::internal
